#version 460 core

in layout(location = 0) vec3 a_position;
in layout(location = 2) vec2 a_texcoord0;

out vec2 Texcoord;

void main()
{
	Texcoord = a_texcoord0;
	gl_Position = vec4(a_position, 1.0);
}