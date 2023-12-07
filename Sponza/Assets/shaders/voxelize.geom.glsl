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

int getDominantAxis(vec3 normal)
{
    vec3 N = abs(normal);

    int maxAxis = N.x > N.y ? DEPTH_AXIS_X : DEPTH_AXIS_Y;
    return N.z > N[maxAxis] ? DEPTH_AXIS_Z : maxAxis;
}

vec3 getSurfaceNormal(vec4 v0, vec4 v1, vec4 v2)
{
    vec3 e0 = v1.xyz - v0.xyz;
    vec3 e1 = v2.xyz - v0.xyz;

    return normalize(cross(e0, e1));
}

vec4 expandVertex(vec4 position, vec3 normal, int axis)
{
    float halfTexelSize = 1.0 / imageSize(u_voxelGrid).x;
    
    position.xyz += normal * halfTexelSize;
    return position;
}

void main()
{
    vec3 normal = getSurfaceNormal(gl_in[0].gl_Position, gl_in[1].gl_Position, gl_in[2].gl_Position);
    int axis = getDominantAxis(normal);

    for(int i = 0; i < gl_in.length(); ++i)
    {
        vec4 expanded = expandVertex(gl_in[i].gl_Position, normal, axis);
        v_texcoord = Texcoord[i];
        v_worldPos = (u_model * expanded).xyz;

        gl_Position = u_proj * u_view * u_model * expanded;

        EmitVertex();
    }

    EndPrimitive();
}
