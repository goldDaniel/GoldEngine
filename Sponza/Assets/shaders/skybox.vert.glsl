#version 460 core

layout (location = 0) in vec3 a_Position;

out vec3 TextureDir;

layout(std140) uniform PerDrawConstants_UBO
{
    mat4 u_model;
		
	// for use when maps are not present
	vec4 u_albedo;
	vec4 u_emissive;
	vec4 u_coefficients;// metallic, roughness, ?, uvScale

	vec4 u_flags; // albedoMap, normalMap, metallicMap, roughnessMap
};

void main()
{
    TextureDir = a_Position;
    vec4 pos = u_model * vec4(a_Position, 1.0);
    gl_Position = pos.xyww;
}  