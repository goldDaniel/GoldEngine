#version 460 core

in layout(location = 0) vec3 a_position;
in layout(location = 1) vec3 a_normal;
in layout(location = 2) vec2 a_texcoord0;

out vec3 v_worldPos;
out vec3 v_worldNormal;
out vec2 v_texCoord;

#include "common/uniforms.glslh"

void main()
{
	vec4 worldPos = u_model * vec4(a_position, 1.0);
	
	v_worldNormal   = normalize((transpose(inverse(mat3(u_model)))) * a_normal);
	v_texCoord      = a_texcoord0;
	v_worldPos      = worldPos.xyz;

	gl_Position = u_proj * u_view * worldPos;
}