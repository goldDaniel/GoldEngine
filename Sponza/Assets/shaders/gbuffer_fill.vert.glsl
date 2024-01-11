#version 460 core

#include "common/uniforms.glslh"

in layout(location = 0) vec3 a_position;
in layout(location = 1) vec3 a_normal;
in layout(location = 2) vec2 a_texcoord0;

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