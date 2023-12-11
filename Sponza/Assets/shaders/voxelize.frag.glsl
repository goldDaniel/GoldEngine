#version 460 core

in vec3 v_worldPos;
in vec3 v_worldNormal;
in vec2 v_texCoord;

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

layout(std140) uniform PerDrawConstants_UBO
{
	mat4 u_model;
	int u_materialID;
};

layout (r32ui) uniform coherent volatile uimage3D u_voxelGrid;

uniform sampler2D u_albedoMap;
uniform sampler2D u_normalMap;
uniform sampler2D u_metallicMap;
uniform sampler2D u_roughnessMap;

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

void main()
{
	const ivec3 IMAGE_SIZE = imageSize(u_voxelGrid); 
	const ivec3 IMAGE_CENTER = IMAGE_SIZE / 2;

	ivec3 voxelCoords = ivec3(v_worldPos) + IMAGE_CENTER;
	voxelCoords = clamp(voxelCoords, ivec3(0), IMAGE_SIZE - ivec3(1));

	ivec2 size = textureSize(u_albedoMap, 0);
	
	int mipsAvailable = textureQueryLevels(u_albedoMap);
	vec4 sampledColor = texture(u_albedoMap, v_texCoord);

	if(sampledColor.a > 0.0)
	{
		vec4 color = vec4(sampledColor.xyz, 1.0 / 256.0);
		imageAtomicAverage(voxelCoords, color);
	}
	else 
	{
		discard;
	}
}
