#version 460 core

in vec3 TextureDir; 

uniform samplerCube u_cubemap; 

out vec4 color0;

void main()
{
	color0 = texture(u_cubemap, TextureDir);
}
