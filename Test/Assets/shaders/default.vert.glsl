#version 460

layout(location = 0) in vec3 a_position;
layout(location = 4) in vec3 a_color;

layout(std140) uniform View_UBO 
{
	mat4 u_mvp;	
};

out vec3 Color;

void main()
{
	Color = a_color;
	gl_Position = u_mvp * vec4(a_position, 1.0);
}