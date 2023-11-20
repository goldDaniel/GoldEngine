#version 460 core

out vec3 color0; // albedo
out vec3 color1; // normal
out vec3 color2; // coefficients

in vec3 Position;
in vec3 Normal;
in vec2 Texcoord;

layout(std140) uniform PerDrawConstants_UBO
{
    mat4 u_model;
	
	int u_materialID;
	int pad[3];
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
uniform sampler2D u_normalMap;
uniform sampler2D u_metallicMap;
uniform sampler2D u_roughnessMap;

vec3 GetWorldSpaceNormal(vec2 texCoords)
{
    vec3 tangentNormal = texture(u_normalMap, texCoords).xyz * 2.0 - 1.0;

    vec3 Q1  = dFdx(Position);
    vec3 Q2  = dFdy(Position);
    vec2 st1 = dFdx(Texcoord);
    vec2 st2 = dFdy(Texcoord);

    vec3 N   = normalize(Normal);
    vec3 T  = normalize(Q1*st2.t - Q2*st1.t);
    vec3 B  = -normalize(cross(N, T));
    mat3 TBN = mat3(T, B, N);

    return normalize(TBN * tangentNormal);
}

void main()
{
    Material material = u_materials[u_materialID];

    vec4 albedoColor = material.albedo;
    if(material.mapFlags.x > 0)
    {
        albedoColor = texture(u_albedoMap, Texcoord.xy * material.coefficients.w);
    }
    if(albedoColor.a < 0.4) discard;
	

    color0 = albedoColor.xyz;
    color1 = normalize(Normal).xyz;
    color2.r = material.coefficients.x;
    color2.g = material.coefficients.y;

    if(material.mapFlags.y > 0)
    {
        color1 = GetWorldSpaceNormal(Texcoord.xy * material.coefficients.w).rgb;
    }

    //note (danielg): sometimes the textures we get have the values in the g channel? 
    if(material.mapFlags.z > 0)
    {
        float value = texture(u_metallicMap, Texcoord.xy * material.coefficients.w).g;
        if(value <= 0) 
        {
            value = texture(u_metallicMap, Texcoord.xy * material.coefficients.w).r;
        }
        color2.r = value;
    }
    if(material.mapFlags.w > 0)
    {
        float value = texture(u_roughnessMap, Texcoord.xy * material.coefficients.w).g;
        if(value <= 0) 
        {
            value = texture(u_roughnessMap, Texcoord.xy * material.coefficients.w).r;
        }
        color2.g = value;
    }
}