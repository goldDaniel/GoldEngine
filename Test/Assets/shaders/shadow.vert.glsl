#version 460 core
layout (location = 0) in vec3 a_position;

layout(std140) uniform PerDrawConstants_UBO
{
    mat4 u_model; // this should hold (lightSpace * model)
		
	// these are all unused here
	vec4 u_albedo;
	vec4 u_emissive;
	vec4 u_coefficients;

	vec4 u_flags;
};

void main()
{
    gl_Position = u_model * vec4(a_position, 1.0);
}  