#include "SceneLoader.h"

#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/Importer.hpp>

#include "scene/BaseComponents.h"

#include "graphics/RenderTypes.h"
#include "graphics/Vertex.h"
#include "graphics/Texture.h"

#include <future>

using namespace scene;

static void CreateMaterial(const std::string& filepath, unsigned int index, aiMaterial** const materials, gold::FrameEncoder& encoder, RenderComponent& render);
static void CreateMesh(const aiMesh* mesh, gold::FrameEncoder& encoder, RenderComponent& render);

static std::unordered_map<u32, graphics::TextureHandle> kTextureCache;

GameObject Loader::LoadGameObjectFromModel(Scene& scene, gold::FrameEncoder& encoder, const std::string& file)
{
	constexpr unsigned int assimpFlags = 0	| aiProcess_Triangulate
											| aiProcess_FlipUVs
											| aiProcess_ImproveCacheLocality
											| aiProcess_RemoveRedundantMaterials
											| aiProcess_JoinIdenticalVertices
											| aiProcess_SplitLargeMeshes
											| aiProcess_OptimizeMeshes;

	Assimp::Importer importer;
	const aiScene* assimpScene = importer.ReadFile(file, assimpFlags);
	DEBUG_ASSERT(assimpScene && assimpScene->HasMeshes(), "Failed to load mesh file");

	std::string filepath = file.substr(0, file.find_last_of('/'));
	filepath += '/';

	
	GameObject parentObject = scene.CreateGameObject(assimpScene->mName.C_Str());
	

	for (u32 i = 0; i < assimpScene->mNumMeshes; ++i)
	{
		GameObject child = scene.CreateGameObject(assimpScene->mMeshes[i]->mName.C_Str());
		child.SetParent(parentObject);
		RenderComponent& render = child.AddComponent<RenderComponent>();

		CreateMesh(assimpScene->mMeshes[i], encoder, render);
		CreateMaterial(filepath, assimpScene->mMeshes[i]->mMaterialIndex, assimpScene->mMaterials, encoder, render);
	}

	return parentObject;
}

static void CreateMesh(const aiMesh* mesh, gold::FrameEncoder& encoder, RenderComponent& render)
{
	using namespace graphics;

	DEBUG_ASSERT(mesh->HasPositions(), "Mesh does not have positions!");
	DEBUG_ASSERT(mesh->HasNormals(), "Mesh does not have normals!");
	DEBUG_ASSERT(mesh->HasTextureCoords(0), "Mesh does not have texture coordinates!");

	auto posBuffer = VertexBuffer(std::move(VertexLayout().Push<VertexLayout::Position3>()));
	auto norBuffer = VertexBuffer(std::move(VertexLayout().Push<VertexLayout::Normal>()));
	auto texBuffer = VertexBuffer(std::move(VertexLayout().Push<VertexLayout::Texcoord2>()));
	posBuffer.Reserve(mesh->mNumVertices);
	norBuffer.Reserve(mesh->mNumVertices);
	texBuffer.Reserve(mesh->mNumVertices);

	constexpr float fMin = std::numeric_limits<float>::min();
	constexpr float fMax = std::numeric_limits<float>::max();
	render.aabbMin = { fMax,fMax,fMax };
	render.aabbMax = { fMin, fMin, fMin};

	for (size_t i = 0; i < mesh->mNumVertices; ++i)
	{
		glm::vec3 pos = { mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z };

		if (pos.x < render.aabbMin.x) render.aabbMin.x = pos.x;
		if (pos.y < render.aabbMin.y) render.aabbMin.y = pos.y;
		if (pos.z < render.aabbMin.z) render.aabbMin.z = pos.z;

		if (pos.x > render.aabbMax.x) render.aabbMax.x = pos.x;
		if (pos.y > render.aabbMax.y) render.aabbMax.y = pos.y;
		if (pos.z > render.aabbMax.z) render.aabbMax.z = pos.z;

		auto nor = glm::vec3{ mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z };
		auto tex = glm::vec2{ mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y };

		posBuffer.Emplace(std::move(pos));
		norBuffer.Emplace(std::move(nor));
		texBuffer.Emplace(std::move(tex));
	}

	DEBUG_ASSERT(posBuffer.VertexCount() == mesh->mNumVertices &&
				 norBuffer.VertexCount() == mesh->mNumVertices &&
				 texBuffer.VertexCount() == mesh->mNumVertices, "");

	std::vector<u32> indices;

	indices.reserve(mesh->mNumFaces);
	for (size_t i = 0; i < mesh->mNumFaces; ++i)
	{
		DEBUG_ASSERT(mesh->mFaces[i].mNumIndices == 3, "");
		indices.push_back(mesh->mFaces[i].mIndices[0]);
		indices.push_back(mesh->mFaces[i].mIndices[1]);
		indices.push_back(mesh->mFaces[i].mIndices[2]);
	}

	graphics::MeshDescription desc{};

	desc.handles.mPositions = encoder.CreateVertexBuffer(posBuffer.Raw(), posBuffer.SizeInBytes());
	desc.handles.mNormals = encoder.CreateVertexBuffer(norBuffer.Raw(), norBuffer.SizeInBytes());
	desc.handles.mTexCoords0 = encoder.CreateVertexBuffer(texBuffer.Raw(), texBuffer.SizeInBytes());

	desc.mVertexCount = posBuffer.VertexCount();

	if (indices.size() > 0)
	{
		desc.mIndicesFormat = IndexFormat::U32;
		desc.mIndices = encoder.CreateIndexBuffer(indices.data(), indices.size() * sizeof(u32));
		desc.mIndexCount = static_cast<uint32_t>(indices.size());
	}

	render.mesh = encoder.CreateMesh(desc);
}

static std::mutex kTextureWriteMutex;
static graphics::TextureHandle FindOrAddTexture(const std::string& file, gold::FrameEncoder& encoder)
{
	graphics::Texture2D texture(file);
	if (kTextureCache.find(texture.GetNameHash()) == kTextureCache.end())
	{
		graphics::TextureDescription2D desc(texture, true);
		{
			std::scoped_lock lock(kTextureWriteMutex);
			kTextureCache[texture.GetNameHash()] = encoder.CreateTexture2D(desc);
		}
		
	}

	return kTextureCache.find(texture.GetNameHash())->second;
}

static void CreateMaterial(const std::string& filepath, unsigned int index, aiMaterial** const materials, gold::FrameEncoder& encoder, RenderComponent& render)
{
	const auto material = materials[index];

	aiString albedo;
	aiString normal;
	aiString metalic;
	aiString roughness;
	
	if (material->GetTexture(AI_MATKEY_BASE_COLOR_TEXTURE, &albedo) == AI_SUCCESS || 
		material->GetTexture(aiTextureType_DIFFUSE,0, &albedo) || 
		material->GetTexture(aiTextureType_AMBIENT, 0, &albedo))
	{
		render.albedoMap = FindOrAddTexture(filepath + albedo.C_Str(), encoder);	
	}

	if (material->GetTexture(aiTextureType_NORMALS, 0, &normal) == AI_SUCCESS)
	{
		render.normalMap = FindOrAddTexture(filepath + normal.C_Str(), encoder);
	}

	if (material->GetTexture(AI_MATKEY_METALLIC_TEXTURE, &metalic) == AI_SUCCESS)
	{
		render.metallicMap = FindOrAddTexture(filepath + metalic.C_Str(), encoder);
	}

	if (material->GetTexture(AI_MATKEY_ROUGHNESS_TEXTURE, &roughness) == AI_SUCCESS)
	{
		render.roughnessMap = FindOrAddTexture(filepath + roughness.C_Str(), encoder);	
	}
	
	if (!render.albedoMap.idx)
	{
		aiColor4D albedoVal;
		if (aiGetMaterialColor(material, AI_MATKEY_BASE_COLOR, &albedoVal) == AI_SUCCESS)
		{
			render.albedo = glm::vec4(albedoVal.r, albedoVal.g, albedoVal.b, 1.0f);
		}
	}

	if (!render.metallicMap.idx)
	{

		float metalicVal;
		if (aiGetMaterialFloat(material, AI_MATKEY_METALLIC_FACTOR, &metalicVal) == AI_SUCCESS)
		{
			render.metallic = metalicVal;
		}
	}

	if (!render.roughnessMap.idx)
	{
		float roughnessVal;
		if (aiGetMaterialFloat(material, AI_MATKEY_ROUGHNESS_FACTOR, &roughnessVal) == AI_SUCCESS)
		{
			render.roughness = roughnessVal;
		}
	}
}
