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
		
	// for use when maps are not present
	vec4 u_albedo;
	vec4 u_emissive;
	vec4 u_coefficients;// metallic, roughness, ?, uvScale

	vec4 u_flags; // albedoMap, normalMap, metallicMap, roughnessMap
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
    vec4 albedoColor = u_albedo;
    if(u_flags.x > 0)
    {
        albedoColor = texture(u_albedoMap, Texcoord.xy * u_coefficients.w);
    }
    if(albedoColor.a < 0.4) discard;
	

    color0 = albedoColor.xyz;
    color1 = normalize(Normal).xyz;// * 0.5 + 0.5;
    color2.r = u_coefficients.r;
    color2.g = u_coefficients.g;

    if(u_flags.y > 0)
    {
        color1 = GetWorldSpaceNormal(Texcoord.xy * u_coefficients.w).rgb;
    }

    //note (danielg): sometimes the textures we get have the values in the g channel? 
    if(u_flags.z > 0)
    {
        float value = texture(u_metallicMap, Texcoord.xy * u_coefficients.w).g;
        if(value <= 0) 
        {
            value = texture(u_metallicMap, Texcoord.xy * u_coefficients.w).r;
        }
        color2.r = value;
    }
    if(u_flags.w > 0)
    {
        float value = texture(u_roughnessMap, Texcoord.xy * u_coefficients.w).g;
        if(value <= 0) 
        {
            value = texture(u_roughnessMap, Texcoord.xy * u_coefficients.w).r;
        }
        color2.g = value;
    }
}