#include "SceneLoader.h"

using namespace scene;

void Loader::LoadGameObjectFromModel(Scene& scene, gold::FrameEncoder& encoder, std::string& filepath)
{
	constexpr unsigned int assimpFlags = 0 | aiProcess_Triangulate
		| aiProcess_FlipUVs;

	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(filename, assimpFlags);
	assert(scene && scene->HasMeshes() && "Failed to load mesh file");

	std::string filepath = filename.substr(0, filename.find_last_of('/'));
	filepath += '/';

	for (size_t i = 0; i < scene->mNumMeshes; ++i)
	{
		MeshMaterialPair pair;
		pair.mesh = CreateMesh(scene->mMeshes[i]);

		pair.material = CreateMaterial(filepath, scene->mMeshes[i]->mMaterialIndex, scene->mMaterials);

		result.push_back(pair);
	}

	return result;
}