#pragma once

#include "core/Core.h"

#include "Material.h"

class MaterialManager
{
public:
	static constexpr u32 MAX_MATERIALS = 128;

private:
	bool mDirty{ false };
	graphics::MaterialHandle mNextHandle{ 0 };
	std::array<graphics::Material, MAX_MATERIALS> mMaterials;
public:

	graphics::MaterialHandle CreateMaterial()
	{
		DEBUG_ASSERT(mNextHandle.idx < MAX_MATERIALS, "Material count exceeded");
		return { mNextHandle.idx++ };
	}

	bool CheckSetDirty()
	{
		bool result = mDirty;
		mDirty = false;

		return result;
	}

	void UpdateMaterial(graphics::MaterialHandle handle, graphics::Material& material)
	{
		mDirty = true;
		mMaterials[handle.idx] = material;
	}

	graphics::Material GetMaterial(graphics::MaterialHandle handle) const
	{
		return mMaterials[handle.idx];
	}

	const auto& GetMaterials() const 
	{
		return mMaterials;
	}
};