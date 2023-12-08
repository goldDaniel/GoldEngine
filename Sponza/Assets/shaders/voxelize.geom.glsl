#version 460 core

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out; 

in vec2 Texcoord[];

out vec3 v_worldPos;
out vec2 v_texcoord;

layout(std140) uniform PerFrameConstants_UBO
{
    mat4 u_proj;
	mat4 u_projInv;
    
	mat4 u_view;
	mat4 u_viewInv;

    vec4 u_time;
};
 
layout(std140) uniform PerDrawConstants_UBO
{
    mat4 u_model;
    int u_materialID;
};

layout (r32ui) uniform coherent volatile uimage3D u_voxelGrid;

const float Epsilon = 0.001;


const int DEPTH_AXIS_X = 0;
const int DEPTH_AXIS_Y = 1;
const int DEPTH_AXIS_Z = 2;

vec3 getViewDirection(mat4 viewMatrix) 
{
    vec3 forward = -vec3(viewMatrix[0][2], viewMatrix[1][2], viewMatrix[2][2]);
    return normalize(forward);
}

int getDominantAxis(vec3 normal)
{
    vec3 N = abs(normal);

    int maxAxis = N.x > N.y ? DEPTH_AXIS_X : DEPTH_AXIS_Y;
    return N.z > N[maxAxis] ? DEPTH_AXIS_Z : maxAxis;
}

vec3 getSurfaceNormal(vec3 v0, vec3 v1, vec3 v2)
{
    vec3 e0 = v1.xyz - v0.xyz;
    vec3 e1 = v2.xyz - v0.xyz;

    return normalize(cross(e0, e1));
}


vec4 calculateClipSpaceOffset(vec4 clipSpacePos, float offsetScale) 
{
    vec2 direction = normalize(clipSpacePos.xy);
    return vec4(direction * offsetScale, 0.0, 0.0);
}

void main()
{
    // assumption, voxel size should be uniform (NxNxN grid)
    const int IMAGE_SIZE = imageSize(u_voxelGrid).x;
    const float HALF_PIXEL_SIZE = 1.0 / float(IMAGE_SIZE);

    vec3 worldPos[3];
    vec3 centeroid = vec3(0);
    for(int i = 0; i < gl_in.length(); ++i)
    {
        worldPos[i] = (u_model * gl_in[i].gl_Position).xyz;
        centeroid += worldPos[i];
    }
    centeroid /= gl_in.length(); // optimization, move divide out of loop

    vec3 normal = getSurfaceNormal(worldPos[0], worldPos[1], worldPos[2]);
    int axis = getDominantAxis(normal);

    vec3 viewDir = getViewDirection(u_view);
    bool isEdgeOn = abs(dot(normal, viewDir)) < Epsilon;

    // Emit each vertex with slight expansion
    for (int i = 0; i < gl_in.length(); i++) {
        // orthographically project along dominant axis
        // use centeroid of triangle to maintain approximate world space pos
        // if(axis == DEPTH_AXIS_X)        worldPos[i].x = centeroid.x;
        // else if(axis == DEPTH_AXIS_Y)   worldPos[i].y = centeroid.y;
        // else if(axis == DEPTH_AXIS_Z)   worldPos[i].z = centeroid.z;
        
        vec4 clip = u_proj * u_view * vec4(worldPos[i], 1.0);

        float expansionFactor = isEdgeOn ? HALF_PIXEL_SIZE * 2 : HALF_PIXEL_SIZE;
        if(axis == DEPTH_AXIS_X && abs(viewDir.x) > 0 && 
           axis == DEPTH_AXIS_Y && abs(viewDir.y) > 0 && 
           axis == DEPTH_AXIS_Z && abs(viewDir.z) > 0)
        {
            expansionFactor *= 2;
        }
        clip += calculateClipSpaceOffset(clip, expansionFactor);


        gl_Position = clip;
        v_worldPos = (u_viewInv * u_projInv * clip).xyz;
        v_texcoord = Texcoord[i];
        EmitVertex();
    }
    EndPrimitive();
}
