#version 460 core 

layout(r32ui) uniform writeonly uimage3D u_voxelGrid;

layout(local_size_x = 8, local_size_y = 8, local_size_z = 8) in;
void main()
{
    ivec3 voxelCoords = ivec3(gl_GlobalInvocationID.xyz);
    ivec3 gridSize = imageSize(u_voxelGrid);

    if(voxelCoords.x < gridSize.x && voxelCoords.y < gridSize.y && voxelCoords.z < gridSize.z)
    {
        imageStore(u_voxelGrid, voxelCoords, uvec4(0,0,0,0));
    }
}