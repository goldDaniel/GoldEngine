#version 460 core

in vec4 v_aabb;
in vec2 v_texCoord;
in flat int v_depthAxis;

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
	const float Epsilon = 0.001;
	const int DEPTH_AXIS_X = 0;
	const int DEPTH_AXIS_Y = 1;
	const int DEPTH_AXIS_Z = 2;

	const ivec3 IMAGE_SIZE = imageSize(u_voxelGrid); 

	// ndc
	vec2 pos = (gl_FragCoord.xy / vec2(IMAGE_SIZE.xy)) * 2.0 - 1.0;

	// AABB bounds clamp
	if(	pos.x < v_aabb.x || pos.y < v_aabb.y || 
		pos.x > v_aabb.z || pos.y > v_aabb.w)
	{
		discard;
	}
	
	float depth = float(IMAGE_SIZE.z - 1);
	ivec3 voxelPos;

	voxelPos.xy = ivec2(gl_FragCoord.xy);
	voxelPos.z = int(depth * gl_FragCoord.z);

	float halfDepthRange = abs(fwidth(gl_FragCoord.z) * 0.5);

	int depthStart = int(depth * clamp(gl_FragCoord.z - halfDepthRange, 0.0, 1.0));
	int depthEnd   = int(depth * clamp(gl_FragCoord.z + halfDepthRange, 0.0, 1.0));
	ivec3 voxelRangeStart = ivec3(voxelPos.xy, depthStart);
	ivec3 voxelRangeEnd   = ivec3(voxelPos.xy, depthEnd);

	int voxelRange = voxelRangeEnd.z - voxelRangeStart.z + 1;

	ivec3 stepAxis = ivec3(0,0,1);

	if(v_depthAxis == DEPTH_AXIS_X)
	{
		ivec3 temp;
		temp.x = IMAGE_SIZE.x - voxelRangeStart.z;
		temp.z = voxelRangeStart.x;

		voxelRangeStart.xz = temp.xz;
		stepAxis = ivec3(-1,0,0);
	}
	else if(v_depthAxis == DEPTH_AXIS_Y)
	{
		ivec3 temp;
		temp.y = IMAGE_SIZE.y - voxelRangeStart.z;
		temp.z = voxelRangeStart.y;

		voxelRangeStart.yz = temp.yz;

		stepAxis = ivec3(0,-1,0);
	}

	for (int i = 0; i < voxelRange; i++)
	{
		vec4 color = vec4(texture(u_albedoMap, v_texCoord).xyz, 1.0 / 256.0);
		imageAtomicAverage(voxelRangeStart, color);
		voxelRangeStart += stepAxis; 
	}
}