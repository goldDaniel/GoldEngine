#version 460 core

in vec2 Texcoord;

uniform sampler2D u_albedoMap;

#include "common/uniforms.glslh"

void main()
{
    Material material = u_materials[u_materialID];
    if(material.mapFlags.x > 0)
    {
        if(texture(u_albedoMap, Texcoord).a < 0.4)
        {
            discard;
        }
    }       
    gl_FragDepth = gl_FragCoord.z;
}  