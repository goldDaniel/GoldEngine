#version 460 core

in layout(location = 0) vec3 a_Position;
in layout(location = 2) vec2 a_Texcoord;

out vec2 Texcoord;

void main()
{	
	Texcoord = a_Texcoord;
	gl_Position = vec4(a_Position, 1.0);
}