#version 460 core

layout (location = 0) in vec3 a_Position;
layout (location = 2) in vec2 a_Texcoord0;

out vec2 Texcoord;

void main()
{
	Texcoord = a_Texcoord0;
	gl_Position = vec4(a_Position, 1);
}