#version 460 core
layout (location = 0) in vec3 a_position;
layout (location = 2) in vec2 a_texcoord;

#include "common/uniforms.glslh"

out vec2 Texcoord;

void main()
{
	Texcoord = a_texcoord;
	gl_Position = u_model * vec4(a_position, 1.0);
}  