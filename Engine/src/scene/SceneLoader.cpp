#include "SceneLoader.h"

#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/Importer.hpp>

#include "scene/BaseComponents.h"

#include "graphics/RenderTypes.h"
#include "graphics/Vertex.h"
#include "graphics/Texture.h"

using namespace scene;

static void CreateMaterial(const std::string& filepath, unsigned int index, aiMaterial** const materials, gold::FrameEncoder& encoder, RenderComponent& render);
static void CreateMesh(const aiMesh* mesh, gold::FrameEncoder& encoder, RenderComponent& render);

static u32 index = 0;
static Assimp::Importer importer;
static const aiScene* assimpScene = nullptr;
static GameObject parentObject;
static std::string filename;
static std::string filepath;
static Loader::Status kStatus = Loader::Status::None;

Loader::Status Loader::LoadGameObjectFromModel(Scene& scene, gold::FrameEncoder& encoder, const std::string& file)
{
	if (kStatus == Status::None)
	{
		index = 0;
		importer.~Importer();
		new(&importer)Assimp::Importer();

		kStatus = Status::Loading;
		filename = file;
		
		constexpr unsigned int assimpFlags = 0	| aiProcess_Triangulate
												| aiProcess_FlipUVs;

		assimpScene = importer.ReadFile(filename, assimpFlags);
		DEBUG_ASSERT(assimpScene && assimpScene->HasMeshes(), "Failed to load mesh file");

		filepath = filename.substr(0, filename.find_last_of('/'));
		filepath += '/';

		parentObject = scene.CreateGameObject(assimpScene->mName.C_Str());
		parentObject.GetComponent<TransformComponent>().scale = { 0.05, 0.05, 0.05 };
	}
	else if (kStatus == Status::Loading)
	{
		DEBUG_ASSERT(file == filename, "Attempting to load a mesh when previous is not complete");
	}

	if (index < assimpScene->mNumMeshes)
	{
		GameObject child = scene.CreateGameObject(assimpScene->mMeshes[index]->mName.C_Str());
		child.SetParent(parentObject);
		RenderComponent& render = child.AddComponent<RenderComponent>();

		CreateMesh(assimpScene->mMeshes[index], encoder, render);
		CreateMaterial(filepath, assimpScene->mMeshes[index]->mMaterialIndex, assimpScene->mMaterials, encoder, render);
		index++;
	}
	else
	{
		kStatus = Status::Finished;
	}

	return kStatus;
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

	for (size_t i = 0; i < mesh->mNumVertices; ++i)
	{
		auto pos = glm::vec3{ mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z };
		auto nor = glm::vec3{ mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z };
		auto tex = glm::vec2{ mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y };

		posBuffer.Emplace(std::move(pos));
		norBuffer.Emplace(std::move(nor));
		texBuffer.Emplace(std::move(tex));
	}

	DEBUG_ASSERT(posBuffer.VertexCount() == mesh->mNumVertices &&
				 norBuffer.VertexCount() == mesh->mNumVertices &&
				 texBuffer.VertexCount() == mesh->mNumVertices, "");

	std::vector<u32> faces;

	faces.reserve(mesh->mNumFaces);
	for (size_t i = 0; i < mesh->mNumFaces; ++i)
	{
		DEBUG_ASSERT(mesh->mFaces[i].mNumIndices == 3, "");
		faces.push_back(mesh->mFaces[i].mIndices[0]);
		faces.push_back(mesh->mFaces[i].mIndices[1]);
		faces.push_back(mesh->mFaces[i].mIndices[2]);
	}

	auto desc = graphics::MeshDescription();

	desc.handles.mPositions = encoder.CreateVertexBuffer(posBuffer.Raw(), posBuffer.SizeInBytes());
	desc.handles.mNormals = encoder.CreateVertexBuffer(norBuffer.Raw(), norBuffer.SizeInBytes());
	desc.handles.mTexCoords0 = encoder.CreateVertexBuffer(texBuffer.Raw(), texBuffer.SizeInBytes());

	desc.mVertexCount = posBuffer.VertexCount();

	if (faces.size() > 0)
	{
		desc.mIndicesFormat = IndexFormat::U32;
		desc.mIndices = encoder.CreateIndexBuffer(faces.data(), faces.size() * sizeof(u32));
		desc.mIndexCount = static_cast<uint32_t>(faces.size());
	}

	render.mesh = encoder.CreateMesh(desc);
}

static void CreateMaterial(const std::string& filepath, unsigned int index, aiMaterial** const materials, gold::FrameEncoder& encoder, RenderComponent& render)
{
	const auto material = materials[index];

	aiString albedo;
	aiString normal;
	aiString metalic;
	aiString roughness;


	if (material->GetTexture(AI_MATKEY_BASE_COLOR_TEXTURE, &albedo) == AI_SUCCESS)
	{
		render.useAlbedoMap = true;

		graphics::Texture2D texture(filepath + albedo.C_Str());
		graphics::TextureDescription2D desc(texture, true);
		render.albedoMap = encoder.CreateTexture2D(desc);
	}

	if (material->GetTexture(aiTextureType_NORMALS, 0, &normal) == AI_SUCCESS)
	{
		render.useNormalMap = true;

		graphics::Texture2D texture(filepath + normal.C_Str());
		graphics::TextureDescription2D desc(texture, true);
		render.normalMap = encoder.CreateTexture2D(desc);
	}

	if (material->GetTexture(AI_MATKEY_METALLIC_TEXTURE, &metalic) == AI_SUCCESS)
	{
		render.useMetallicMap = true;

		graphics::Texture2D texture(filepath + metalic.C_Str());
		graphics::TextureDescription2D desc(texture, true);
		render.metallicMap = encoder.CreateTexture2D(desc);
	}

	if (material->GetTexture(AI_MATKEY_ROUGHNESS_TEXTURE, &roughness) == AI_SUCCESS)
	{
		render.useRoughnessMap = true;

		graphics::Texture2D texture(filepath + roughness.C_Str());
		graphics::TextureDescription2D desc(texture, true);
		render.roughnessMap = encoder.CreateTexture2D(desc);
	}

	if (!render.useAlbedoMap)
	{
		aiColor4D albedoVal;
		if (aiGetMaterialColor(material, AI_MATKEY_BASE_COLOR, &albedoVal) == AI_SUCCESS)
		{
			render.albedo = glm::vec4(albedoVal.r, albedoVal.g, albedoVal.b, 1.0f);
		}
	}

	if (!render.useMetallicMap)
	{

		float metalicVal;
		if (aiGetMaterialFloat(material, AI_MATKEY_METALLIC_FACTOR, &metalicVal) == AI_SUCCESS)
		{
			render.metallic = metalicVal;
		}
	}

	if (!render.useRoughnessMap)
	{
		float roughnessVal;
		if (aiGetMaterialFloat(material, AI_MATKEY_ROUGHNESS_FACTOR, &roughnessVal) == AI_SUCCESS)
		{
			render.roughness = roughnessVal;
		}
	}
}
