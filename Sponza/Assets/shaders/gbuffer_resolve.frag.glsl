#version 460 core

out vec4 color0;
in vec2 Texcoord;

uniform sampler2D u_shadowMap;

uniform sampler3D u_voxelGrid;

uniform sampler2D u_albedos;
uniform sampler2D u_normals;
uniform sampler2D u_coefficients;
uniform sampler2D u_depth;

#include "common/uniforms.glslh"
#include "common/common.glslh"
#include "common/direct_lighting.glslh"
#include "common/indirect_lighting.glslh"


void main()
{
	vec3 albedo     = texture(u_albedos, Texcoord).rgb;
	vec3 normal     = normalize(texture(u_normals, Texcoord).xyz);
	float metallic  = texture(u_coefficients, Texcoord).r;
	float roughness = texture(u_coefficients, Texcoord).g;

	float d = texture(u_depth, Texcoord).r;
	vec3 position = WorldPosFromDepth(Texcoord, d);

	vec3 V = normalize(u_viewPos.xyz - position);
	vec3 F0 = mix(vec3(0.04), albedo.rgb, metallic);
	vec3 Lo = vec3(0.0);

	for(uint i = 0; i < lightCounts.x; ++i)
	{
		Lo += getDirectionalLightContribution(i, albedo, normal, metallic, roughness, d, position, V, F0);
	}

	uvec2 lightBin = getLightBin(Texcoord);
	for(uint i = lightBin.x; i < lightBin.y; ++i) 
	{
		uint lightIndex = uint(u_lightBinIndices[i]);
		Lo += getPointLightContribution(lightIndex, albedo, normal, metallic, roughness, d, position, V, F0);
	}

	if(u_enableGI)
	{
		vec3 reflectDir = reflect(-V, normal);

		Lo += albedo * getIndirectDiffuseContribution(position, normal, roughness);
		Lo += albedo * getIndirectSpecularContribution(position, reflectDir, roughness) * 4;
	}

	vec3 ambient = 0.0 / 256.0 * albedo.rgb;
	vec3 color = ambient + (Lo);
	color0 = vec4(color, 1.0);

	gl_FragDepth = d;
}