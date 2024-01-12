#version 460 core

out vec4 color0;
  
in vec2 Texcoord;

uniform sampler2D hdrBuffer;

void main()
{             
    const float gamma = 2.2;
    vec3 hdrColor = texture(hdrBuffer, Texcoord).rgb;
  
    // reinhard tone mapping
    vec3 mapped = vec3(1.0) - exp(-hdrColor * 2.1);
    // gamma correction 
    mapped = pow(mapped, vec3(1.0 / gamma));
  
    color0 = vec4(mapped, 1.0);
}