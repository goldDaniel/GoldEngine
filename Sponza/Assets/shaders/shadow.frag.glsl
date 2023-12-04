#version 460 core

in vec2 Texcoord;

layout(std140) uniform PerDrawConstants_UBO
{
    mat4 u_model;	
	int u_materialID;
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

uniform sampler2D u_albedoMap;

void main()
{
    Material material = u_materials[u_materialID];
    if(material.mapFlags.x > 0)
    {
        if(texture(u_albedoMap, Texcoord).a < 0.4)
        {
            discard;
        }
    }       
    gl_FragDepth = gl_FragCoord.z;
}  