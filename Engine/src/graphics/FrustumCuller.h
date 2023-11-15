#pragma once 

#include "core/Core.h"
#include "scene/Camera.h"

struct AABB
{
	glm::vec3 min;
	glm::vec3 max;
};

class FrustumCuller
{
public:
	
	//https://iquilezles.org/articles/frustumcorrect/
	static bool AABBInsideFrustum(	const std::array<glm::vec4, 6>& planes,
									const std::array<glm::vec4, 8>& corners,
									const AABB& aabb)
	{
		for (int i = 0; i < 6; i++) 
		{
			int r = 0;
			r += (glm::dot(planes[i], glm::vec4(aabb.min.x, aabb.min.y, aabb.min.z, 1.f)) < 0) ? 1 : 0;
			r += (glm::dot(planes[i], glm::vec4(aabb.max.x, aabb.min.y, aabb.min.z, 1.f)) < 0) ? 1 : 0;
			r += (glm::dot(planes[i], glm::vec4(aabb.min.x, aabb.max.y, aabb.min.z, 1.f)) < 0) ? 1 : 0;
			r += (glm::dot(planes[i], glm::vec4(aabb.max.x, aabb.max.y, aabb.min.z, 1.f)) < 0) ? 1 : 0;
			r += (glm::dot(planes[i], glm::vec4(aabb.min.x, aabb.min.y, aabb.max.z, 1.f)) < 0) ? 1 : 0;
			r += (glm::dot(planes[i], glm::vec4(aabb.max.x, aabb.min.y, aabb.max.z, 1.f)) < 0) ? 1 : 0;
			r += (glm::dot(planes[i], glm::vec4(aabb.min.x, aabb.max.y, aabb.max.z, 1.f)) < 0) ? 1 : 0;
			r += (glm::dot(planes[i], glm::vec4(aabb.max.x, aabb.max.y, aabb.max.z, 1.f)) < 0) ? 1 : 0;
			if (r == 8) return false;
		}

		{
			int r = 0;
			for (int i = 0; i < 8; i++) r += ((corners[i].x > aabb.max.x) ? 1 : 0);
			if (r == 8) return false;
		}
		{
			int r = 0; 
			for (int i = 0; i < 8; i++) r += ((corners[i].x < aabb.min.x) ? 1 : 0);
			if (r == 8) return false;
		}
		{
			int r = 0; 
			for (int i = 0; i < 8; i++) r += ((corners[i].y > aabb.max.y) ? 1 : 0);
			if (r == 8) return false;
		}
		{
			int r = 0; 
			for (int i = 0; i < 8; i++) r += ((corners[i].y < aabb.min.y) ? 1 : 0);
			if (r == 8) return false;
		}
		{
			int r = 0; 
			for (int i = 0; i < 8; i++) r += ((corners[i].z > aabb.max.z) ? 1 : 0);
			if (r == 8) return false;
		}
		{
			int r = 0; 
			for (int i = 0; i < 8; i++) r += ((corners[i].z < aabb.min.z) ? 1 : 0);
			if (r == 8) return false;
		}

		return true;
	}

	static bool FrustumCulled(const glm::mat4& viewProjInv, const glm::mat4& viewProjTrans, const AABB& aabb)
	{
		std::array<glm::vec4, 6> frustumPlanes =
		{
			viewProjTrans[3] + viewProjTrans[0],
			viewProjTrans[3] - viewProjTrans[0],
			viewProjTrans[3] + viewProjTrans[1],
			viewProjTrans[3] - viewProjTrans[1],
			viewProjTrans[3] + viewProjTrans[2],
			viewProjTrans[3] - viewProjTrans[2],
		};

		std::array<glm::vec4, 8> frustumCorners =
		{
			glm::vec4(-1, -1, -1, 1),
			glm::vec4( 1, -1, -1, 1),
			glm::vec4( 1,  1, -1, 1),
			glm::vec4(-1,  1, -1, 1),
			glm::vec4(-1, -1,  1, 1),
			glm::vec4( 1, -1,  1, 1),
			glm::vec4( 1,  1,  1, 1),
			glm::vec4(-1,  1,  1, 1)
		};

		for (auto& corner : frustumCorners)
		{
			glm::vec4 q = viewProjInv * corner;
			corner = q / q.w;
		}

		return !AABBInsideFrustum(frustumPlanes, frustumCorners, aabb);
	}
};
