#version 460 core

out vec4 color0;
  
in vec2 Texcoord;

uniform sampler2D hdrBuffer;

float luminance(vec3 v)
{
    return dot(v, vec3(0.2126f, 0.7152f, 0.0722f));
}

vec3 change_luminance(vec3 c_in, float l_out)
{
    float l_in = luminance(c_in);
    return c_in * (l_out / l_in);
}

// TMOS /////////////////////////////////////////////////////

vec3 reinhard(vec3 v)
{
    return v / (1.0 + v);
}

vec3 reinhardExtended(vec3 v, float max_white)
{
    vec3 numerator = v * (1.0 + (v / vec3(max_white * max_white)));
    return numerator / (1.0 + v);
}

vec3 reinhardExtendedLuminance(vec3 v, float max_white_l)
{
    float l_old = luminance(v);
    float numerator = l_old * (1.0 + (l_old / (max_white_l * max_white_l)));
    float l_new = numerator / (1.0 + l_old);
    return change_luminance(v, l_new);
}



////////////////////////////////////////////////////////

void main()
{
    const float gamma = 2.2;
    vec3 hdrColor = texture(hdrBuffer, Texcoord).rgb;
  
    // reinhard tone mappinga
    vec3 mapped = reinhardExtendedLuminance(hdrColor, 1);

    // gamma correction 
    mapped = pow(mapped, vec3(1.0 / gamma));
  
    color0 = vec4(mapped, 1.0);
}