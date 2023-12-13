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

const int lightsPerBin = 8;
layout(std140) readonly buffer LightBins_UBO
{
	uvec4 u_binsCounts; //x,y,z, ?
	uvec4 u_lightBins[]; // start, end, pad, pad
	
};

layout(std430) readonly buffer LightBinIndices_UBO
{
	int u_lightBinIndices[]; 
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

	vec4 u_viewPos;
	vec4 u_time;
};

out vec4 color0;

in vec2 Texcoord;


vec3 colorAdjust = vec3(0,0,0);

uniform sampler2D shadowMap;

uniform sampler3D u_voxelGrid;

uniform sampler2D albedos;
uniform sampler2D normals;
uniform sampler2D coefficients;
uniform sampler2D depth;

vec2 mapValue(vec2 x, vec2 in_min, vec2 in_max, vec2 out_min, vec2 out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

uvec2 getLightBin(vec2 screenPos) 
{
	uvec2 bin = ivec2(0, 0);

	int numBinsX = int(u_binsCounts.x + 0.5);
	int numBinsY = int(u_binsCounts.y + 0.5);
	int binX = clamp(int(screenPos.x * u_binsCounts.x), 0, numBinsX - 1);
	int binY = clamp(int(screenPos.y * u_binsCounts.y), 0, numBinsY - 1);

	int binIndex = (binY * numBinsX) + binX;
	bin.x = u_lightBins[binIndex].x;
	bin.y = u_lightBins[binIndex].y;
	
	return bin;
}

vec3 mipToColor(float mipLevel)
{
	vec3 colors[] =
	{
		vec3(1.0 / 7.0),
		vec3(1.0 / 6.0),
		vec3(1.0 / 5.0),
		vec3(1.0 / 4.0),
		vec3(1.0 / 3.0),
		vec3(1.0 / 2.0),
		vec3(1.0 / 1.0),
	};

	int level = clamp(int(round(mipLevel)), 0, 7);
	return colors[level];
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
	float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + Epsilon;
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

	// just in case zero is sent down. but PCFSize should always be >= 1
	int pcfSize = max(1, int(shadowMapParams[shadowMapIndex].z));
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
	uvec2 lightBin = getLightBin(Texcoord);
	for(uint idx = lightBin.x; idx < lightBin.y; ++idx) 
	{
		uint lightIndex = uint(u_lightBinIndices[idx]);

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

vec3 coneTrace(vec3 worldPos, vec3 direction, float aperture)
{
	const int numMips = textureQueryLevels(u_voxelGrid);
	const float VOXEL_GRID_SIZE = textureSize(u_voxelGrid, 0).x;
	const float VOXEL_SIZE_BASE = 1;

	vec4 result = vec4(0);
	
	float currentMip = 0;
	float coneCoeff = 2.0 * tan(aperture * 0.5);

	float voxelSize = 1 * exp2(currentMip);

	vec3 start = worldPos + (VOXEL_SIZE_BASE * 2) * direction;
	
	const float minRadius = VOXEL_SIZE_BASE * VOXEL_GRID_SIZE * 0.5;

	float s = 0;
	float diameter = max(s * coneCoeff, VOXEL_SIZE_BASE);
	vec3 weight = direction*direction;
	float currSegmentLength = voxelSize;

	while (s < VOXEL_GRID_SIZE)
	{
		vec3 position = start + direction * s;
		ivec3 voxelCoords = ivec3(position) + ivec3(VOXEL_GRID_SIZE / 2);

		float distanceToCenter = length(voxelCoords - ivec3(VOXEL_GRID_SIZE / 2.0));
		float minMip = ceil(log2(distanceToCenter / minRadius));

		currentMip = log2(diameter / VOXEL_SIZE_BASE);
		currentMip = min(max(max(0, currentMip), minMip), numMips - 1);

		vec3 uvw = vec3(voxelCoords) / vec3(VOXEL_GRID_SIZE);
		float floatBits = textureLod(u_voxelGrid, uvw, currentMip).r;
		uint packedColor = floatBitsToUint(floatBits);
		vec4 color = unpackUnorm4x8(packedColor);
		
		voxelSize = VOXEL_SIZE_BASE * exp2(currentMip);

		// Radiance correction
		float correctionQuotient = currSegmentLength / voxelSize;
		color.rgb = color.rgb * correctionQuotient;
		
		if (color.a > 0)
		{
			result += color;
		}

		float sLast = s;
		s += max(diameter, VOXEL_SIZE_BASE) * 0.2;
		currSegmentLength = (s - sLast);
		diameter = s * coneCoeff;
	}

	return result.rgb;
}

const int NUM_DIFFUSE_CONES = 32;
const vec3 DIFFUSE_CONE_DIRECTIONS[32] = 
{
    vec3(0.898904, 0.435512, 0.0479745),
    vec3(0.898904, -0.435512, -0.0479745),
    vec3(0.898904, 0.0479745, -0.435512),
    vec3(0.898904, -0.0479745, 0.435512),
    vec3(-0.898904, 0.435512, -0.0479745),
    vec3(-0.898904, -0.435512, 0.0479745),
    vec3(-0.898904, 0.0479745, 0.435512),
    vec3(-0.898904, -0.0479745, -0.435512),
    vec3(0.0479745, 0.898904, 0.435512),
    vec3(-0.0479745, 0.898904, -0.435512),
    vec3(-0.435512, 0.898904, 0.0479745),
    vec3(0.435512, 0.898904, -0.0479745),
    vec3(-0.0479745, -0.898904, 0.435512),
    vec3(0.0479745, -0.898904, -0.435512),
    vec3(0.435512, -0.898904, 0.0479745),
    vec3(-0.435512, -0.898904, -0.0479745),
    vec3(0.435512, 0.0479745, 0.898904),
    vec3(-0.435512, -0.0479745, 0.898904),
    vec3(0.0479745, -0.435512, 0.898904),
    vec3(-0.0479745, 0.435512, 0.898904),
    vec3(0.435512, -0.0479745, -0.898904),
    vec3(-0.435512, 0.0479745, -0.898904),
    vec3(0.0479745, 0.435512, -0.898904),
    vec3(-0.0479745, -0.435512, -0.898904),
    vec3(0.57735, 0.57735, 0.57735),
    vec3(0.57735, 0.57735, -0.57735),
    vec3(0.57735, -0.57735, 0.57735),
    vec3(0.57735, -0.57735, -0.57735),
    vec3(-0.57735, 0.57735, 0.57735),
    vec3(-0.57735, 0.57735, -0.57735),
    vec3(-0.57735, -0.57735, 0.57735),
    vec3(-0.57735, -0.57735, -0.57735)
};

vec3 getIndirectDiffuseContribution(vec3 worldPos, vec3 normal)
{
    float aperture = 0.628319;
	vec3 result = vec3(0.0);

	for(int i = 0; i < NUM_DIFFUSE_CONES; ++i)
	{
		float cosTheta = dot(normal, DIFFUSE_CONE_DIRECTIONS[i]);
		if (cosTheta > 0)
		{
			result += coneTrace(worldPos, DIFFUSE_CONE_DIRECTIONS[i], aperture) * cosTheta;
		}
	}
	result /= NUM_DIFFUSE_CONES * 0.5;

	return result;
}

vec3 getIndirectSpecularContribution(vec3 worldPos, vec3 direction, float roughness)
{
	float aperture = max(roughness, 0.01);
	return coneTrace(worldPos, direction, aperture);
}
void main()
{
	vec3 albedo     = texture(albedos, Texcoord).rgb;
	vec3 normal     = normalize(texture(normals, Texcoord).xyz);
	float metallic  = texture(coefficients, Texcoord).r;
	float roughness = texture(coefficients, Texcoord).g;

	float d = texture(depth, Texcoord).r;
	vec3 position = WorldPosFromDepth(d);

	vec3 V = normalize(u_viewPos.xyz - position);
	vec3 reflectDir = (-V);
	vec3 F0 = mix(vec3(0.04), albedo.rgb, metallic);
	vec3 Lo = vec3(0.0);

	Lo += getDirectionalLightContribution(albedo, normal, metallic, roughness, d, position, V, F0);
	Lo += getPointLightContribution(albedo, normal, metallic, roughness, d, position, V, F0);
	Lo += getIndirectDiffuseContribution(position, normal);
	Lo += getIndirectSpecularContribution(position, reflectDir, roughness);

	vec3 ambient = 0.0 / 256.0 * albedo.rgb;
	vec3 color = ambient + (Lo);
	color0 = vec4(color, 1.0);

	gl_FragDepth = d;
	
}