#version 460 core

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out; 

in vec2 Texcoord[];

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
	int pad[3];
};

layout (r32ui) uniform coherent volatile uimage3D u_voxelGrid;

const float Epsilon = 0.001;


const int DEPTH_AXIS_X = 0;
const int DEPTH_AXIS_Y = 1;
const int DEPTH_AXIS_Z = 2;

int dominantAxis(vec3 normal)
{
    vec3 N = abs(normal);

    int maxAxis = N.x > N.y ? DEPTH_AXIS_X : DEPTH_AXIS_Y;
    return N.z > N[maxAxis] ? DEPTH_AXIS_Z : maxAxis;
}

void main()
{
    // expand triangle for conservative rastization
    for(int i = 0; i < 3; ++i)
    {
        gl_Position = u_proj * u_view * u_model * vec4(gl_in[i].gl_Position.xyz, 1.0);
        v_texcoord = Texcoord[i];
        EmitVertex();
    }
    EndPrimitive();
}
