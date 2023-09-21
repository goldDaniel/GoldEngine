#version 460

in vec3 Color;

out vec4 color0;

void main()
{
	color0 = vec4(Color, 1.0);
}