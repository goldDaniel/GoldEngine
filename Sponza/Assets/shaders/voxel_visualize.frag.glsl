#version 460 core

layout(std140) uniform PerFrameConstants_UBO
{
	mat4 u_proj;
	mat4 u_projInv;
	
	mat4 u_view;
	mat4 u_viewInv;

	vec4 u_viewPos;
	vec4 u_time;
};

layout(r32ui) uniform readonly uimage3D u_voxelGrid;

in vec2 Texcoord;
out vec4 color0;

struct Ray
{
	vec3 origin;
	vec3 dir;
};

Ray GetFragmentRay(vec2 NDC)
{
	Ray ray;
	ray.origin = u_viewInv[3].xyz;

	vec4 clip = vec4(NDC, -1.0, 1.0);
    
	vec4 eye = u_projInv * clip;
    eye = vec4(eye.xy, -1.0, 0.0);
    
    vec4 world = u_viewInv * eye;
    ray.dir = normalize(world.xyz);

	return ray;
}

void main()
{
    const ivec3 IMAGE_SIZE = ivec3(512);
    const float STEP_SIZE = 0.5;
    const float STEP_SIZE_INV = 1.0 / STEP_SIZE;
    const uint STEP_COUNT = uint(STEP_SIZE_INV * IMAGE_SIZE.x);

    const vec2 uv = Texcoord * 2.0 - 1.0;
    Ray ray = GetFragmentRay(uv);
    
    for(int i = 0; i < STEP_COUNT; ++i)
    {
        const vec3 worldPos = ray.origin + STEP_SIZE * i * ray.dir;
        ivec3 voxelPos = ivec3(worldPos) + (IMAGE_SIZE / 2);
        voxelPos /= int(pow(2, u_time.z));

        if(voxelPos.x >= 0 && voxelPos.y >= 0 && voxelPos.z >= 0 &&
           voxelPos.x < IMAGE_SIZE.x && voxelPos.y < IMAGE_SIZE.y && voxelPos.z < IMAGE_SIZE.z)
        {
            uint packedVoxel = imageLoad(u_voxelGrid, voxelPos).r;
            vec4 unpackedColor = unpackUnorm4x8(packedVoxel);
            if(unpackedColor.a > 0)
            {
                color0 = vec4(unpackedColor.rgb, 1.0);
                return;
            }
        }
    }

    discard;
}