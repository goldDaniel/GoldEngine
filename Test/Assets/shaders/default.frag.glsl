#version 460

in vec3 Color;
in vec2 TexCoords;

out vec4 color0;

uniform sampler2D u_texture;

void main()
{
	color0 = vec4(texture(u_texture, TexCoords).rgb, 1.0);
}