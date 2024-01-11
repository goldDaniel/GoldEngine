#version 460 core

out vec3 color0; // albedo
out vec3 color1; // normal
out vec3 color2; // coefficients

in vec3 Position;
in vec3 Normal;
in vec2 Texcoord;

uniform sampler2D u_albedoMap;
uniform sampler2D u_normalMap;
uniform sampler2D u_metallicMap;
uniform sampler2D u_roughnessMap;

#include "common/uniforms.glslh"
#include "common/common.glslh"
#include "common/material_sampling.glslh"

void main()
{
    Material material = u_materials[u_materialID];

    vec4 albedo = getAlbedo(material, Texcoord);
    if(albedo.a < 0.4) discard;

    color0   = albedo.rgb;
    color1   = getNormal(material, Position, Normal, Texcoord);
    color2.r = getMetallic( material, Texcoord);
    color2.g = getRoughness(material, Texcoord);
}