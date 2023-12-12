#version 460 core 

// NOTE (danielg): Since the we store the RGBA value where the alpha channel stores how many fragments 
//                 wrote into the voxel during voxelization step, and it is encoded as a u32
//                 using glGenerateTextureMipmap() will give us incorrect results.


layout(r32ui) uniform readonly uimage3D u_voxelGridUpper;
layout(r32ui) uniform writeonly uimage3D u_voxelGridLower;

bool boundsCheck(ivec3 coord, ivec3 dim)
{
    return (coord.x >= 0 && coord.x < dim.x && 
            coord.y >= 0 && coord.y < dim.y && 
            coord.z >= 0 && coord.z < dim.z);
}

layout(local_size_x = 8, local_size_y = 8, local_size_z = 8) in;
void main()
{
    ivec3 lowerDim = imageSize(u_voxelGridLower);
    ivec3 dstCoords = ivec3(gl_GlobalInvocationID.xyz);

    if(!boundsCheck(dstCoords, lowerDim))
    {
        return;
    }

    ivec3 upperDim = imageSize(u_voxelGridUpper);
    ivec3 srcCoords[8];
    srcCoords[0] = ivec3(gl_GlobalInvocationID.xyz * 2) + ivec3(0, 0, 0);
    srcCoords[1] = ivec3(gl_GlobalInvocationID.xyz * 2) + ivec3(0, 1, 0);
    srcCoords[2] = ivec3(gl_GlobalInvocationID.xyz * 2) + ivec3(1, 0, 0);
    srcCoords[3] = ivec3(gl_GlobalInvocationID.xyz * 2) + ivec3(1, 1, 0);
    srcCoords[4] = ivec3(gl_GlobalInvocationID.xyz * 2) + ivec3(0, 0, 1);
    srcCoords[5] = ivec3(gl_GlobalInvocationID.xyz * 2) + ivec3(0, 1, 1);
    srcCoords[6] = ivec3(gl_GlobalInvocationID.xyz * 2) + ivec3(1, 0, 1);
    srcCoords[7] = ivec3(gl_GlobalInvocationID.xyz * 2) + ivec3(1, 1, 1);

    vec4 downsampledColor = vec4(0);
    for(int i = 0; i < 8; ++i)
    {
        if(boundsCheck(srcCoords[i], upperDim))
        {
            uint packedColor = imageLoad(u_voxelGridUpper, srcCoords[i]).r;
            vec4 unpackedColor = unpackUnorm4x8(packedColor); 

            if(unpackedColor.a > 0)
            {
                downsampledColor.rgb += unpackedColor.rgb;
                downsampledColor.a += 1;
            }
        }
    }

    if(downsampledColor.a > 0)
    {
        downsampledColor.rgb /= downsampledColor.a;
    }

    // no longer care about weight, just RGB values. alpha is used as a flag to see if voxel is filled
    uint packedResult = packUnorm4x8(downsampledColor);
    imageStore(u_voxelGridLower, dstCoords, uvec4(packedResult, 0,0,0));
}