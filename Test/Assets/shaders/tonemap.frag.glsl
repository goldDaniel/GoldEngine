#version 460 core

out vec4 color0;
  
in vec2 Texcoord;

layout(std140) uniform Exposure
{
    vec4 exposure; // exposure, ?, ?, ?
};

uniform sampler2D hdrBuffer;

void main()
{             
    const float gamma = 2.2;
    vec3 hdrColor = texture(hdrBuffer, Texcoord).rgb;
  
    // reinhard tone mapping
    vec3 mapped = vec3(1.0) - exp(-hdrColor * exposure.x);
    // gamma correction 
    mapped = pow(mapped, vec3(1.0 / gamma));
  
    color0 = vec4(mapped, 1.0);
}  