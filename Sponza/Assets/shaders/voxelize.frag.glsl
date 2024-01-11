#version 460 core

in vec3 v_worldPos;
in vec3 v_worldNormal;
in vec2 v_texCoord;

layout (r32ui) uniform coherent volatile uimage3D u_voxelGrid;

uniform sampler2D u_shadowMap;

uniform sampler2D u_albedoMap;
uniform sampler2D u_normalMap;
uniform sampler2D u_metallicMap;
uniform sampler2D u_roughnessMap;

#include "common/uniforms.glslh"
#include "common/common.glslh"
#include "common/material_sampling.glslh"
#include "common/direct_lighting.glslh"

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
	vec4 albedo     = getAlbedo(material, v_texCoord);
	vec3 normal     = getNormal(material, v_worldPos, v_worldNormal, v_texCoord);
	float metallic  = getMetallic(material, v_texCoord);
	float roughness = getRoughness(material, v_texCoord);

	if(albedo.a > 0)
	{
		vec3 position = v_worldPos;
		
		vec3 V = normalize(u_viewPos.xyz - position);
		vec3 F0 = mix(vec3(0.04), albedo.rgb, metallic);
		vec3 Lo = vec3(0.0);
		
		for(uint i = 0; i < lightCounts.x; ++i)
		{
			Lo += getDirectionalLightContribution(i, albedo.rgb, normal, metallic, roughness, gl_FragCoord.z, position, V, F0);
		}
		for(uint i = 0; i < lightCounts.y; ++i)
		{
			Lo += getPointLightContribution(i, albedo.rgb, normal, metallic, roughness, gl_FragCoord.z, position, V, F0);
		}

		imageAtomicAverage(voxelCoords, vec4(Lo, 1.0/256.0));
	}
	else
	{
		discard;
	}
}
