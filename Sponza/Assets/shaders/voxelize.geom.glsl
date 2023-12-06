#version 460 core

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out; 

in vec2 Texcoord[];

out vec4 v_aabb;
out vec2 v_texcoord;
out flat int v_depthAxis;

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

int dominantAxis(vec3 normal)
{
    vec3 N = abs(normal);

    int maxAxis = N.x > N.y ? DEPTH_AXIS_X : DEPTH_AXIS_Y;
    return N.z > N[maxAxis] ? DEPTH_AXIS_Z : maxAxis;
}

vec3 getSurfaceNormal(vec4 vertices[3])
{
    vec3 e0 = vertices[1].xyz - vertices[0].xyz;
    vec3 e1 = vertices[2].xyz - vertices[0].xyz;

    return normalize(cross(e0, e1));
}

void main()
{
    float HALF_TEXEL_SIZE = 1.0 / imageSize(u_voxelGrid).x;

    vec4 NDC[3];
    vec2 TexcoordOut[3];

    for(int i = 0; i < gl_in.length(); ++i)
    {
        NDC[i] = u_proj * u_view * u_model * gl_in[i].gl_Position;
        NDC[i] /= NDC[i].w;

        TexcoordOut[i] = Texcoord[i];
    }

    vec3 normal = getSurfaceNormal(NDC);
    int depthAxis = dominantAxis(normal);
    v_depthAxis = depthAxis;

    if(depthAxis == DEPTH_AXIS_Y)
    {
        vec3 temp;
        for (int i = 0; i < gl_in.length(); i++)
        {
            temp.y = NDC[i].z;
            temp.z = -NDC[i].y;

            NDC[i].yz = temp.yz; 
        }
    }
    else if(depthAxis == DEPTH_AXIS_X)
    {
        vec3 temp;
        for (int i = 0; i < gl_in.length(); i++)
        {
            temp.x = NDC[i].z;
            temp.z = -NDC[i].x; 
            
            NDC[i].xz = temp.xz; 
        }
    }

    // swap winding for backface
    normal = getSurfaceNormal(NDC);
    if(dot(normal, vec3(0,0,1)) < 0.0)
    {
        vec4 vertexTemp = NDC[2];
        vec2 texCoordTemp = TexcoordOut[2];
        
        NDC[2] = NDC[1];
        TexcoordOut[2] = TexcoordOut[1];
    
        NDC[1] = vertexTemp;
        TexcoordOut[1] = texCoordTemp;
    }

    vec4 trianglePlane; //a,b,c,d
    trianglePlane.xyz = getSurfaceNormal(NDC);
    trianglePlane.w = -dot(NDC[0].xyz, trianglePlane.xyz);

    if(trianglePlane.z == 0)
    {
        return;
    }

    //in NDC. xy is min, yw is max
    vec4 aabb = vec4(1,1,-1,-1);
    for(int i = 0; i < gl_in.length(); ++i)
    {
        aabb.xy = min(aabb.xy, NDC[i].xy);
        aabb.zw = max(aabb.zw, NDC[i].xy);
    }

    v_aabb = aabb = vec4(-HALF_TEXEL_SIZE, -HALF_TEXEL_SIZE, HALF_TEXEL_SIZE, HALF_TEXEL_SIZE);


    vec3 plane[3];       
    for (int i = 0; i < gl_in.length(); i++)
    {
		plane[i] = cross(NDC[i].xyw, NDC[(i + 2) % 3].xyw);
		plane[i].z -= dot(vec2(HALF_TEXEL_SIZE,HALF_TEXEL_SIZE), abs(plane[i].xy));
    }


    vec3 intersect[3];
    for (int i = 0; i < gl_in.length(); i++)
    {
        intersect[i] = cross(plane[i], plane[(i+1) % 3]);   
        if (intersect[i].z == 0.0)
        {
            return;
        }
        intersect[i] /= intersect[i].z; 
    }

    for(int i = 0; i < gl_in.length(); ++i)
    {
        gl_Position.xyw = intersect[i];
        gl_Position.z = -(trianglePlane.x * intersect[i].x + trianglePlane.y * intersect[i].y + trianglePlane.w) / trianglePlane.z;   
        v_texcoord = TexcoordOut[i];        

        EmitVertex();
    }

    EndPrimitive();
}
