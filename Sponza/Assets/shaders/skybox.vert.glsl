#version 460 core

layout (location = 0) in vec3 a_Position;

out vec3 TextureDir;

#include "common/uniforms.glslh"

void main()
{
    TextureDir = a_Position;
    vec4 pos = u_model * vec4(a_Position, 1.0);
    gl_Position = pos.xyww;
}  