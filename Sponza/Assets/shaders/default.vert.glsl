#version 460

layout(location = 0) in vec3 a_position;
layout(location = 2) in vec2 a_texcoords0;
layout(location = 4) in vec3 a_color;

layout(std140) uniform View_UBO 
{
	mat4 u_mvp;	
};

out vec3 Color;
out vec2 TexCoords;

void main()
{
	Color = a_color;
	TexCoords = a_texcoords0;
	gl_Position = u_mvp * vec4(a_position, 1.0);
}