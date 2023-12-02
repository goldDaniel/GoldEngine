#version 460 core

in vec2 v_texcoord;

struct Material
{
	vec4 albedo;
	vec4 emissive;

    //metallic, roughness, ?, UVScale
	vec4 coefficients;

	// albedo, normal, metallic, roughness
	uvec4 mapFlags;
};

#define MAX_MATERIALS 128
layout(std140) uniform Materials_UBO
{
    Material u_materials[MAX_MATERIALS];
};

layout(std140) uniform PerDrawConstants_UBO
{
    mat4 u_model;
	
	int u_materialID;
	int pad[3];
};

layout (r32ui) uniform coherent volatile uimage3D u_voxelGrid;

uniform sampler2D u_albedoMap;
uniform sampler2D u_normalMap;
uniform sampler2D u_metallicMap;
uniform sampler2D u_roughnessMap;

void imageAtomicAverage(ivec3 pos, vec4 addingColor)
{
    uint newValue = packUnorm4x8(addingColor);
    uint expectedValue = 0;

    uint actualValue = imageAtomicCompSwap(u_voxelGrid, pos, expectedValue, newValue);

    while (actualValue != expectedValue)
    {
        expectedValue = actualValue;
    
        // Unpack the current average and convert back
        // Add the current color, normalize, repack.
        vec4 color = unpackUnorm4x8(actualValue);
        color.a *= 256;
        color.rgb *= color.a;
                
        color.rgb += addingColor.rgb;
        color.a += 1.0;

        color.rgb /= color.a;
        color.a /= 256;
     
        newValue = packUnorm4x8(color);
        actualValue = imageAtomicCompSwap(u_voxelGrid, pos, expectedValue, newValue);
    }
}

void main()
{
    // xyz SHOULD all be equal so just grab one
    const int IMAGE_SIZE = imageSize(u_voxelGrid).x; 
    
    ivec3 voxelIndex = ivec3(gl_FragCoord.x, gl_FragCoord.y, 0);

    vec4 color = texture(u_albedoMap, v_texcoord);
    imageAtomicAverage(voxelIndex, vec4(1,0,0,1));
}