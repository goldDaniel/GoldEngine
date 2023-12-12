#version 460 core

in layout(location = 0) vec3 a_position;
in layout(location = 1) vec3 a_normal;
in layout(location = 2) vec2 a_texcoord0;


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

out vec3 Position;
out vec3 Normal;
out vec2 Texcoord;

void main()
{
	Normal   = (transpose(inverse(mat3(u_model)))) * a_normal;
	Texcoord = a_texcoord0;
	Position = (u_model * vec4(a_position, 1.0)).xyz;

	gl_Position = u_proj * u_view * vec4(Position, 1.0);
}