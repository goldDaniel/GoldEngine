#version 460 core

in vec3 v_worldPos;
in vec3 v_worldNormal;
in vec2 v_texCoord;

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

struct Material
{
	vec4 albedo;
	vec4 emissive;

	//metallic, roughness, ?, UVScale
	vec4 coefficients;

	// albedo, normal, metallic, roughness
	uvec4 mapFlags;
};

#define MAX_MATERIALS 128
layout(std140) uniform Materials_UBO
{
	Material u_materials[MAX_MATERIALS];
};

layout(std140) uniform PerFrameConstants_UBO
{
	mat4 u_proj;
	mat4 u_projInv;
	
	mat4 u_view;
	mat4 u_viewInv;

	vec4 u_viewPos;
	vec4 u_time;
};

layout(std140) uniform PerDrawConstants_UBO
{
	mat4 u_model;
	int u_materialID;
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

layout (r32ui) uniform coherent volatile uimage3D u_voxelGrid;

uniform sampler2D shadowMap;

uniform sampler2D u_albedoMap;
uniform sampler2D u_normalMap;
uniform sampler2D u_metallicMap;
uniform sampler2D u_roughnessMap;

vec2 mapValue(vec2 x, vec2 in_min, vec2 in_max, vec2 out_min, vec2 out_max)
{
	return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void imageAtomicAverage(ivec3 pos, vec4 addingColor)
{
	uint newValue = packUnorm4x8(addingColor);
	uint expectedValue = 0;

	uint actualValue = imageAtomicCompSwap(u_voxelGrid, pos, expectedValue, newValue);

	while (actualValue != expectedValue)
	{
		expectedValue = actualValue;

		// Unpack the current average and convert back
		// Add the current color, normalize, repack.
		vec4 color = unpackUnorm4x8(actualValue);
		color.a *= 256;
		color.rgb *= color.a;

		color.rgb += addingColor.rgb;
		color.a += 1.0;

		color.rgb /= color.a;
		color.a /= 256;

		newValue = packUnorm4x8(color);
		actualValue = imageAtomicCompSwap(u_voxelGrid, pos, expectedValue, newValue);
	}
}

bool boundsCheck(ivec3 coord, ivec3 dim)
{
	return (coord.x >= 0 && coord.x < dim.x && 
		coord.y >= 0 && coord.y < dim.y && 
		coord.z >= 0 && coord.z < dim.z);
}

vec4 getAlbedo(Material material)
{
	vec4 result = material.albedo;
	if (material.mapFlags.x > 0)
	{
		result = texture(u_albedoMap, v_texCoord.xy * material.coefficients.w);
	}
	return result;
}

vec3 getNormal(Material material)
{
	vec3 result = normalize(v_worldNormal).xyz;
	if (material.mapFlags.y > 0)
	{
		vec3 tangentNormal = texture(u_normalMap, v_texCoord).xyz * 2.0 - 1.0;

		vec3 Q1  = dFdx(v_worldPos);
		vec3 Q2  = dFdy(v_worldPos);
		vec2 st1 = dFdx(v_texCoord);
		vec2 st2 = dFdy(v_texCoord);

		vec3 N   = normalize(v_worldNormal);
		vec3 T   = normalize(Q1*st2.t - Q2*st1.t);
		vec3 B   = -normalize(cross(N, T));
		mat3 TBN = mat3(T, B, N);

		result = normalize(TBN * tangentNormal);
	}

	return result;
}

//note (danielg): sometimes the textures we get have the values in the g channel? 
float getMetallic(Material material)
{
	float result = material.coefficients.x;
	if (material.mapFlags.z > 0)
	{
		result = texture(u_metallicMap, v_texCoord.xy * material.coefficients.w).g;
		if (result <= 0)
		{
			result = texture(u_metallicMap, v_texCoord.xy * material.coefficients.w).r;
		}
	}
	return result;
}

//note (danielg): sometimes the textures we get have the values in the g channel? 
float getRoughness(Material material)
{
	float result = material.coefficients.y;
	if (material.mapFlags.w > 0)
	{
		result = texture(u_roughnessMap, v_texCoord.xy * material.coefficients.w).g;
		if (result <= 0)
		{
			result = texture(u_roughnessMap, v_texCoord.xy * material.coefficients.w).r;
		}
	}
	return result;
}

float getShadowPCF(vec3 projCoords, float NdotL, int shadowMapIndex, float bias)
{
	vec2 shadowMapSize = textureSize(shadowMap, 0);
	vec2 texelSize = 1.0 / shadowMapSize.xy;
	vec4 bounds = shadowMapPage[shadowMapIndex];
	
	vec4 uvBounds = vec4(0,0,0,0);
	uvBounds.xz =  bounds.xz / shadowMapSize.x;
	uvBounds.yw =  bounds.yw / shadowMapSize.y;

	// just in case zero is sent down. but PCFSize should always be >= 1
	int pcfSize = 1;
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
	
	shadow /= (pcfSize*pcfSize);
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
	float bias = max(0.005 * (1.0 - NdotL), shadowMapParams[index].w);
	return getShadowPCF(projCoords, NdotL, index, bias);
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

float getPointShadow(uint pointLightIndex, vec4 fragmentPosWorldSpace, vec3 normal)
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


	float NdotL = dot(normal, normalize(fragPosToLightPos));
	float bias = max(0.0001, shadowMapParams[shadowMapIndex].w);
	return getShadowPCF(projCoords, NdotL, shadowMapIndex, bias);
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
	float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + Epsilon;
	vec3 specular     = numerator / denominator;
			
	float NdotL = max(dot(N, L), 0.0);
	return (kD * albedo.rgb / PI + specular) * radiance * NdotL;
}

vec3 getDirectionalLightContribution(vec3 albedo, vec3 normal, float metallic, float roughness, float depth, vec3 worldPos, vec3 view, vec3 F0)
{
	vec3 Lo = vec3(0);

	// direct lighting (directional lights)
	for(int i = 0; i < lightCounts.x; ++i) 
	{
		vec3 L = normalize(-directionalLights[i].direction.xyz);
		vec3 H = normalize(view + L);
		vec3 radiance = directionalLights[i].color.rgb;
		
		int shadowMapIndex = directionalLights[i].params.w;

		vec3 lighting = getLighting(L, normal, view, H, F0, radiance, albedo.rgb, roughness, metallic);
		if(directionalLights[i].params.w != -1) // if shadowmaps are enabled for this light
		{
			float NdotL = dot(normal, normalize(directionalLights[i].direction.xyz));
			float shadow = getDirectionalShadow(shadowMapIndex, mLightSpace[shadowMapIndex] * vec4(worldPos, 1.0), NdotL);
			lighting *= (1.0 - shadow);
		}

		Lo +=  lighting;
	}

	return Lo;
}

vec3 getPointLightContribution(vec3 albedo, vec3 normal, float metallic, float roughness, float depth, vec3 worldPos, vec3 view, vec3 F0)
{
	vec3 Lo = vec3(0);

	// direct lighting (point lights)
	for(uint idx = 0; idx < lightCounts.y; ++idx) 
	{
		uint lightIndex = idx;

		vec3 L = normalize(pointLights[lightIndex].position.xyz - worldPos);
		vec3 H = normalize(view + L);
		float dist = length(pointLights[lightIndex].position.xyz - worldPos);

		float falloff = pointLights[lightIndex].params0.x;
		{
			float attenuation = 1.0/(dist*dist) - 1.0/(falloff*falloff);

			vec3 radiance = clamp(pointLights[lightIndex].color.rgb * attenuation, 0, 1);

			vec3 lighting = getLighting(L, normal, view, H, F0, radiance, albedo.rgb, roughness, metallic);
			if(pointLights[lightIndex].params0.z != -1) // if shadowmaps are enabled for this light
			{
				float shadow = getPointShadow(lightIndex, vec4(worldPos, 1.0), normal);
				lighting *= (1.0 - shadow);
			}
			Lo +=  lighting;
		}
	}

	return Lo;
}

vec3 getAmbientLightContribution(vec3 albedo)
{
	return albedo * 0.00;
}

void main()
{
	const ivec3 IMAGE_SIZE = imageSize(u_voxelGrid); 
	const ivec3 IMAGE_CENTER = IMAGE_SIZE / 2;

	ivec3 voxelCoords = ivec3(v_worldPos) + IMAGE_CENTER;
	if (!boundsCheck(voxelCoords, IMAGE_SIZE))
	{
		discard;
	}
	
	Material material = u_materials[u_materialID];
	vec4 albedo     = getAlbedo(material);
	vec3 normal     = getNormal(material);
	float metallic  = getMetallic(material);
	float roughness = getRoughness(material);

	if(albedo.a > 0)
	{
		vec3 position = v_worldPos;
		
		vec3 V = normalize(u_viewPos.xyz - position);
		vec3 F0 = mix(vec3(0.04), albedo.rgb, metallic);
		vec3 Lo = vec3(0.0);
		
		Lo += getAmbientLightContribution(albedo.rgb);
		Lo += getDirectionalLightContribution(albedo.rgb, normal, metallic, roughness, gl_FragCoord.z, position, V, F0);
		Lo += getPointLightContribution(albedo.rgb, normal, metallic, roughness, gl_FragCoord.z, position, V, F0);


		imageAtomicAverage(voxelCoords, vec4(Lo, 1.0/256.0));
	}
	else
	{
		discard;
	}
}
