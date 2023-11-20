#version 460 core

const float PI = 3.141592;
const float Epsilon = 0.00001;

struct DirectionalLight
{
	vec4 direction;
	vec4 color;

	ivec4 params; // ?, ?, ?, shadowMapIndex
};

struct PointLight
{
	vec4 position;
	vec4 color; 
	ivec4 params0; // falloff, ?, shadowMapIndex+X, shadowMapIndex-X
	ivec4 params1; // shadowMapIndex+Y, shadowMapIndex-Y, shadowMapIndex+Z, shadowMapIndex-Z
};

//NOTE (danielg): must match light count in LightingSystem
#define MAX_LIGHTS 64
layout(std140) uniform Lights_UBO
{
	ivec4 lightCounts;

	DirectionalLight directionalLights[MAX_LIGHTS];
	PointLight pointLights[MAX_LIGHTS];
};

layout(std140) uniform LightSpaceMatrices_UBO
{
	mat4 mLightSpace[MAX_LIGHTS];
	mat4 mLightInv[MAX_LIGHTS];
};

layout(std140) uniform ShadowPages_UBO
{
	vec4 shadowMapPage[MAX_LIGHTS];
	vec4 shadowMapParams[MAX_LIGHTS]; // ?, ?, PCF Size, bias
};

layout(std140) uniform PerFrameConstants_UBO
{
	mat4 u_proj;
	mat4 u_projInv;
	
	mat4 u_view;
	mat4 u_viewInv;

	vec4 u_time;
};

out vec4 color0;

in vec2 Texcoord;


vec3 colorAdjust = vec3(0,0,0);

uniform sampler2D shadowMap;

uniform sampler2D albedos;
uniform sampler2D normals;
uniform sampler2D coefficients;
uniform sampler2D depth;

vec2 mapValue(vec2 x, vec2 in_min, vec2 in_max, vec2 out_min, vec2 out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

const int CubemapFace_XP = 0;
const int CubemapFace_XN = 1;
const int CubemapFace_YP = 2;
const int CubemapFace_YN = 3;
const int CubemapFace_ZP = 4;
const int CubemapFace_ZN = 5;

vec2 sampleVirtualCubemap(vec3 v, out int faceIndex)
{
	vec3 vAbs = abs(v);
	float ma;
	vec2 uv;
	if(vAbs.z >= vAbs.x && vAbs.z >= vAbs.y)
	{
		faceIndex = v.z < 0 ? CubemapFace_ZN : CubemapFace_ZP;
		ma = 0.5 / vAbs.z;
		uv = vec2(v.z < 0.0 ? -v.x : v.x, -v.y);
	}
	else if(vAbs.y >= vAbs.x)
	{
		faceIndex = v.y < 0 ? CubemapFace_YN : CubemapFace_YP;
		ma = 0.5 / vAbs.y;
		uv = vec2(v.x, v.y < 0.0 ? -v.z : v.z);
	}
	else
	{
		faceIndex = v.x < 0 ? CubemapFace_XN : CubemapFace_XP;
		ma = 0.5 / vAbs.x;
		uv = vec2(v.x < 0.0 ? v.z : -v.z, -v.y);
	}
	return uv * ma + 0.5;
}


vec3 WorldPosFromDepth(float depth) 
{
	float z = depth * 2.0 - 1.0;

	vec4 clipSpacePosition = vec4(Texcoord * 2.0 - 1.0, z, 1.0);
	vec4 viewSpacePosition = u_projInv * clipSpacePosition;

	// Perspective division
	viewSpacePosition /= viewSpacePosition.w;

	vec4 worldSpacePosition = u_viewInv * viewSpacePosition;

	return worldSpacePosition.xyz;
}

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
	float a = roughness*roughness;
	float a2     = a*a;
	float NdotH  = max(dot(N, H), 0.0);
	float NdotH2 = NdotH*NdotH;
	
	float nom    = a2;
	float denom  = (NdotH2 * (a2 - 1.0) + 1.0);
	denom        = PI * denom * denom;
	
	return nom / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
	float r = (roughness + 1.0);
	float k = (r*r) / 8.0;

	float nom   = NdotV;
	float denom = NdotV * (1.0 - k) + k;

	return nom / denom;
}
  
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
	float NdotV = max(dot(N, V), 0.0);
	float NdotL = max(dot(N, L), 0.0);
	float ggx2 = GeometrySchlickGGX(NdotV, roughness);
	float ggx1 = GeometrySchlickGGX(NdotL, roughness);

	return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
	return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 getLighting(vec3 L, vec3 N, vec3 V, vec3 H, vec3 F0, vec3 radiance, vec3 albedo, float roughness, float metalness)
{
	// cook-torrance brdf
	float NDF = DistributionGGX(N, H, roughness);
	float G   = GeometrySmith(N, V, L, roughness);
	vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);
		
	vec3 kS = F;
	vec3 kD = vec3(1.0) - kS;
	kD *= 1.0 - metalness;
		
	vec3 numerator    = NDF * G * F;
	float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
	vec3 specular     = numerator / denominator;
			
	float NdotL = max(dot(N, L), 0.0);
	return (kD * albedo.rgb / PI + specular) * radiance * NdotL;
}

float getShadowPCF(vec3 projCoords, float NdotL, int shadowMapIndex, float bias)
{
	vec2 shadowMapSize = textureSize(shadowMap, 0);
	vec2 texelSize = 1.0 / shadowMapSize.xy;
	vec4 bounds = shadowMapPage[shadowMapIndex];
	
	vec4 uvBounds = vec4(0,0,0,0);
	uvBounds.xz =  bounds.xz / shadowMapSize.x;  
	uvBounds.yw =  bounds.yw / shadowMapSize.y;  

	int pcfSize = int(shadowMapParams[shadowMapIndex].z);
	int halfPCFSize = int(pcfSize / 2.0);

	// PCF
	float shadow = 0.0;
	for(int x = -halfPCFSize; x < -halfPCFSize + pcfSize; ++x)
	{
		for(int y = -halfPCFSize; y < -halfPCFSize + pcfSize; ++y)
		{
			vec2 texCoords = projCoords.xy + vec2(x, y) * texelSize;
			vec2 uv = mapValue(texCoords.xy, vec2(0.0 ,0.0), vec2(1.0, 1.0), uvBounds.xy, uvBounds.xy + uvBounds.zw);

			float pcfDepth = texture(shadowMap, uv).r; 
			shadow += (projCoords.z - bias) > pcfDepth  ? 1.0 : 0.0;
		}
	}
	// just in case zero is sent down. but PCFSize should always be >= 1
	shadow /= max(1.0, pcfSize*pcfSize);
	return shadow;
}

float getDirectionalShadow(int index, vec4 posLightSpace, float NdotL)
{
	// perspective divide
	vec3 projCoords = posLightSpace.xyz / posLightSpace.w;
	projCoords = projCoords * 0.5 + 0.5;
	if(projCoords.z > 1)
	{
		return 0.0;
	}
	float bias = max(0.01 * (1.0 - NdotL), shadowMapParams[index].w);
	return getShadowPCF(projCoords, NdotL, index, bias);
}

float getPointShadow(int pointLightIndex, vec4 fragmentPosWorldSpace, vec3 normal)
{
	 vec3 fragPosToLightPos = fragmentPosWorldSpace.xyz - pointLights[pointLightIndex].position.xyz;
	
	int shadowMapIndex;
	vec2 uv = sampleVirtualCubemap(normalize(fragPosToLightPos), shadowMapIndex);

	switch(shadowMapIndex)
	{
	case CubemapFace_XP:
		//colorAdjust = vec3(1,0,0);
		shadowMapIndex = pointLights[pointLightIndex].params0.z;
		break;
	case CubemapFace_XN:
		//colorAdjust = vec3(0,1,0);
		shadowMapIndex = pointLights[pointLightIndex].params0.w;
		break;
	case CubemapFace_YP:
		//colorAdjust = vec3(0,0,1);
		shadowMapIndex = pointLights[pointLightIndex].params1.x;
		break;
	case CubemapFace_YN:
		//colorAdjust = vec3(1,1,0);
		shadowMapIndex = pointLights[pointLightIndex].params1.y;
		break;
	case CubemapFace_ZP:
		//colorAdjust = vec3(1,0,1);
		shadowMapIndex = pointLights[pointLightIndex].params1.z;
		break;
	case CubemapFace_ZN:
		//colorAdjust = vec3(0,1,1);
		shadowMapIndex = pointLights[pointLightIndex].params1.w;
		break;
	default:
		shadowMapIndex = 0;
		break;
	}

	vec4 posLightSpace = mLightSpace[shadowMapIndex] * fragmentPosWorldSpace;

	vec3 projCoords = posLightSpace.xyz / posLightSpace.w;
	projCoords = projCoords * 0.5 + 0.5;
	///projCoords.xy = 1.0 projCoords.xy;


	float NdotL = dot(normal, normalize(fragPosToLightPos));
	float bias = shadowMapParams[shadowMapIndex].w;
	return getShadowPCF(projCoords, NdotL, shadowMapIndex, bias);
}

void main()
{
	vec4 albedo     = texture(albedos, Texcoord).rgba;
	vec3 normal     = texture(normals, Texcoord).xyz;
	float metallic  = texture(coefficients, Texcoord).r;
	float roughness = texture(coefficients, Texcoord).g;

	float d = texture(depth, Texcoord).r;
	vec3 position = WorldPosFromDepth(d);

	vec3 V = normalize(u_viewInv[3].xyz - position);
	vec3 F0 = vec3(0.04); 
	F0 = mix(F0, albedo.rgb, metallic);
	// reflectance equation
	vec3 Lo = vec3(0.0);

	for(int i = 0; i < lightCounts.x; ++i) 
	{
		vec3 L = normalize(-directionalLights[i].direction.xyz);
		vec3 H = normalize(V + L);
		vec3 radiance = directionalLights[i].color.rgb;
		
		int shadowMapIndex = directionalLights[i].params.w;

		vec3 lighting = getLighting(L, normal, V, H, F0, radiance, albedo.rgb, roughness, metallic);
		if(directionalLights[i].params.w != -1)
		{
			float NdotL = dot(normal, normalize(directionalLights[i].direction.xyz));
			float shadow = getDirectionalShadow(shadowMapIndex, mLightSpace[shadowMapIndex] * vec4(position, 1.0), NdotL);
			lighting *= (1.0 - shadow);
		}

		Lo +=  lighting;
	}

	for(int i = 0; i < lightCounts.y; ++i) 
	{
		vec3 L = normalize(pointLights[i].position.xyz - position);
		vec3 H = normalize(V + L);
		float dist = length(pointLights[i].position.xyz - position);

		
		//if(dist < pointLights[i].params0.x) // is this needed?
		{
			float attenuation = pointLights[i].params0.x / (dist * dist);
			vec3 radiance = clamp(pointLights[i].color.rgb * attenuation, 0, 1);
		
			vec3 lighting = getLighting(L, normal, V, H, F0, radiance, albedo.rgb, roughness, metallic);
			if(pointLights[i].params0.z != -1)
			{
				float shadow = getPointShadow(i, vec4(position, 1.0), normal);
				lighting *= (1.0 - shadow);
			}
			Lo +=  lighting;
		}
	}
	
	vec3 ambient = vec3(0.001) * albedo.rgb;
	vec3 color = ambient + (Lo);

	gl_FragDepth = d;
	color0 = vec4(color, 1.0);
}