#include "AssetProcessor.h"

#include <core/Core.h>
#include <core/Logging.h>

#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/Importer.hpp>

#include <graphics/Vertex.h>
#include <graphics/Texture.h>

#include <memory/Utils.h>
#include <memory/BinaryWriter.h>

#include <algorithm>
#include <random>

using namespace graphics;

struct AssetID
{
	static constexpr u8 KEY_LENGTH = 16;
	char key[KEY_LENGTH + 1];

	bool operator==(const AssetID other)
	{
		for (int i = 0; i < KEY_LENGTH; ++i)
		{
			if (key[i] != other.key[i])
			{
				return false;
			}
		}

		return true;
	}

	const char* Name() const
	{
		return key;
	}
};

struct ParsedMaterial
{
	AssetID id{};

	glm::vec4 albedo{};
	float metallic{};
	float roughness{};

	const Texture2D* albedoMap   { nullptr };
	const Texture2D* normalMap   { nullptr };
	const Texture2D* metallicMap { nullptr };
	const Texture2D* roughnessMap{ nullptr };

	bool operator==(const ParsedMaterial& other)
	{
		return
			albedo == other.albedo &&
			metallic == other.metallic &&
			roughness == other.roughness &&
			albedoMap == other.albedoMap &&
			normalMap == other.normalMap &&
			metallicMap == other.metallicMap &&
			roughnessMap == other.roughnessMap;
	}
};

struct ParsedMesh
{
	AssetID id;

	glm::vec3 aabbMin{};
	glm::vec3 aabbMax{};

	VertexBuffer interlacedVertices{};
	std::vector<u32> indices{};

	AssetID materialID{};
};

struct ParsedModel
{
	AssetID id;
	std::vector<ParsedMesh> meshes;
};

static const Texture2D* FindOrAddTexture(const std::string& file)
{
	static std::unordered_map<std::string, Texture2D> loadedTextures;

	if (loadedTextures.find(file) == loadedTextures.end())
	{
		loadedTextures.emplace(file, file);
	}

	return &loadedTextures[file];
}


AssetID createAssetID()
{
	static std::random_device dev;
	static std::mt19937 rng(dev());

	std::uniform_int_distribution<int> dist(0, 15);
	const char* v = "0123456789abcdef";

	AssetID result;
	for (int i = 0; i < AssetID::KEY_LENGTH; i++)
	{
		result.key[i] = v[dist(rng)];
		result.key[i] = v[dist(rng)];
	}
	
	result.key[AssetID::KEY_LENGTH] = '\0';

	return result;
}

static ParsedMesh CreateMesh(const aiMesh* mesh)
{
	using namespace graphics;

	DEBUG_ASSERT(mesh->HasPositions(), "Mesh does not have positions!");
	DEBUG_ASSERT(mesh->HasNormals(), "Mesh does not have normals!");
	DEBUG_ASSERT(mesh->HasTextureCoords(0), "Mesh does not have texture coordinates!");

	ParsedMesh result;
	result.id = createAssetID();

	result.interlacedVertices = VertexBuffer(std::move(VertexLayout().Push<VertexLayout::Position3>()
																	.Push<VertexLayout::Normal>()
																	.Push<VertexLayout::Texcoord2>()));

	result.interlacedVertices.Reserve(mesh->mNumVertices);

	constexpr float fMin = std::numeric_limits<float>::min();
	constexpr float fMax = std::numeric_limits<float>::max();
	result.aabbMin = { fMax,fMax,fMax };
	result.aabbMax = { fMin, fMin, fMin };

	for (size_t i = 0; i < mesh->mNumVertices; ++i)
	{
		glm::vec3 pos = { mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z };

		if (pos.x < result.aabbMin.x) result.aabbMin.x = pos.x;
		if (pos.y < result.aabbMin.y) result.aabbMin.y = pos.y;
		if (pos.z < result.aabbMin.z) result.aabbMin.z = pos.z;

		if (pos.x > result.aabbMax.x) result.aabbMax.x = pos.x;
		if (pos.y > result.aabbMax.y) result.aabbMax.y = pos.y;
		if (pos.z > result.aabbMax.z) result.aabbMax.z = pos.z;

		auto nor = glm::vec3{ mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z };
		auto tex = glm::vec2{ mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y };

		result.interlacedVertices.Emplace(pos, nor, tex);
	}

	DEBUG_ASSERT(result.interlacedVertices.VertexCount() == mesh->mNumVertices, "");

	
	result.indices.reserve(mesh->mNumFaces);
	for (size_t i = 0; i < mesh->mNumFaces; ++i)
	{
		DEBUG_ASSERT(mesh->mFaces[i].mNumIndices == 3, "");
		result.indices.push_back(mesh->mFaces[i].mIndices[0]);
		result.indices.push_back(mesh->mFaces[i].mIndices[1]);
		result.indices.push_back(mesh->mFaces[i].mIndices[2]);
	}

	return result;
}

static ParsedMaterial CreateMaterial(const std::string& filepath, const aiMaterial* const material)
{
	ParsedMaterial result;
	result.id = createAssetID();

	aiString albedo;
	aiString normal;
	aiString metallic;
	aiString roughness;

	// albedo
	if (material->GetTexture(AI_MATKEY_BASE_COLOR_TEXTURE, &albedo) == AI_SUCCESS ||
		material->GetTexture(aiTextureType_DIFFUSE, 0, &albedo) ||
		material->GetTexture(aiTextureType_AMBIENT, 0, &albedo))
	{
		result.albedoMap = FindOrAddTexture(filepath + albedo.C_Str());
	}
	else
	{
		aiColor4D albedoVal;
		if (aiGetMaterialColor(material, AI_MATKEY_BASE_COLOR, &albedoVal) == AI_SUCCESS)
		{
			result.albedo = glm::vec4(albedoVal.r, albedoVal.g, albedoVal.b, 1.0f);
		}
	}

	// normal
	if (material->GetTexture(aiTextureType_NORMALS, 0, &normal) == AI_SUCCESS)
	{
		result.normalMap = FindOrAddTexture(filepath + normal.C_Str());
	}

	// metallic
	if (material->GetTexture(AI_MATKEY_METALLIC_TEXTURE, &metallic) == AI_SUCCESS)
	{
		result.metallicMap = FindOrAddTexture(filepath + metallic.C_Str());
	}
	else
	{
		float metallicVal;
		if (aiGetMaterialFloat(material, AI_MATKEY_METALLIC_FACTOR, &metallicVal) == AI_SUCCESS)
		{
			result.metallic = metallicVal;
		}
	}

	// roughness
	if (material->GetTexture(AI_MATKEY_ROUGHNESS_TEXTURE, &roughness) == AI_SUCCESS)
	{
		result.roughnessMap = FindOrAddTexture(filepath + roughness.C_Str());
	}
	else
	{
		float roughnessVal;
		if (aiGetMaterialFloat(material, AI_MATKEY_ROUGHNESS_FACTOR, &roughnessVal) == AI_SUCCESS)
		{
			result.roughness = roughnessVal;
		}
	}

	return result;
}

static void WriteModelFile(const ParsedModel& model, const std::unordered_map<u32, ParsedMaterial>& materials, const std::string& fileName)
{
	// TODO (danielg): currently assets have a max 2GB buffer. Fix this? calculate it? 
	constexpr u32 bufferSize = 2 * gold::memory::GB;
	u8* buffer = (u8*)malloc(bufferSize);
	gold::BinaryWriter writer(buffer, bufferSize);
	
	writer.Write(model.id);
	
	// write materials
	writer.Write(static_cast<u16>(materials.size()));

	auto writeTexture = [](const Texture2D& tex, gold::BinaryWriter& writer)
	{
		writer.Write(tex.GetWidth());
		writer.Write(tex.GetHeight());
		writer.Write(tex.GetChannels());
		
		const u32 dataSize = tex.GetDataSize();
		const void* data = tex.GetData();

		writer.Write(dataSize);
		writer.Write(data, dataSize);
	};

	for (const auto& entry : materials)
	{
		const auto& material = entry.second;
		G_INFO("Formatting material: {}", material.id.Name());
		writer.Write(material.id);

		{
			u8 hasAlbedoTex = material.albedoMap != nullptr;
			writer.Write(hasAlbedoTex);
			if (hasAlbedoTex) writeTexture(*material.albedoMap, writer);
			else			  writer.Write(material.albedo);
		}

		{
			u8 hasNormalTex = material.normalMap != nullptr;
			writer.Write(hasNormalTex);
			if (hasNormalTex) writeTexture(*material.normalMap, writer);
		}

		{
			u8 hasMetallicTex = material.metallicMap != nullptr;
			writer.Write(hasMetallicTex);
			if (hasMetallicTex) writeTexture(*material.metallicMap, writer);
			else				writer.Write(material.metallic);
		}


		{
			u8 hasRoughnessTex = material.roughnessMap != nullptr;
			writer.Write(hasRoughnessTex);
			if (hasRoughnessTex) writeTexture(*material.roughnessMap, writer);
			else				 writer.Write(material.roughness);
		}
	}

	writer.Write(static_cast<u16>(model.meshes.size()));
	for (const ParsedMesh& mesh : model.meshes)
	{
		G_INFO("Formatting mesh: {}", mesh.id.Name());
		writer.Write(mesh.id);

		//aabb
		writer.Write(mesh.aabbMin);
		writer.Write(mesh.aabbMax);

		// vertex layout
		VertexLayout layout = mesh.interlacedVertices.GetLayout();
		writer.Write(layout.ElementCount());
		for (u32 i = 0; i < layout.ElementCount(); ++i)
		{
			auto element = layout.Resolve(i);
			writer.Write(element.GetType());
		}

		// vertex data
		writer.Write(mesh.interlacedVertices.SizeInBytes());
		writer.Write(mesh.interlacedVertices.Raw(), mesh.interlacedVertices.SizeInBytes());

		// index data
		writer.Write(mesh.indices.size());
		writer.Write(mesh.indices.data(), mesh.indices.size() * sizeof(u32));

		writer.Write(mesh.materialID);
	}
	gold::BinaryReader reader = writer.ToReader();

	G_INFO("Formatting complete. Now Writing to file: {}", fileName);

	std::ofstream stream(fileName, std::ios::out | std::ios::binary | std::ios::trunc);

	stream.write((char*)reader.GetData(), reader.GetSize());
	free(buffer);
}

void assets::ProcessModelAsset(const char* inputFile, const char* outputFile)
{
	G_INFO("Processing model file: {}", inputFile);

	constexpr unsigned int assimpFlags = 0	| aiProcess_Triangulate
											| aiProcess_FlipUVs
											| aiProcess_ImproveCacheLocality
											| aiProcess_RemoveRedundantMaterials
											| aiProcess_JoinIdenticalVertices
											| aiProcess_SplitLargeMeshes
											| aiProcess_OptimizeMeshes;

	Assimp::Importer importer;
	const aiScene* assimpScene = importer.ReadFile(inputFile, assimpFlags);
	DEBUG_ASSERT(assimpScene && assimpScene->HasMeshes(), "Failed to load mesh file");
	if (!assimpScene)
	{
		G_INFO("Failed to load file. Exiting.");
		return;
	}
	G_INFO("Successfully loaded file. Begin parsing.");

	std::string file(inputFile);
	std::replace(file.begin(), file.end(), '\\', '/');

	std::string filepath = file.substr(0, file.find_last_of('/'));
	filepath += '/';

	ParsedModel result;
	result.id = createAssetID();

	std::unordered_map<u32, ParsedMaterial> materials;
	for (u32 i = 0; i < assimpScene->mNumMeshes; ++i)
	{
		ParsedMesh mesh = CreateMesh(assimpScene->mMeshes[i]);
		result.meshes.push_back(mesh);
		G_INFO("Mesh {} parsed", i);

		if (materials.find(assimpScene->mMeshes[i]->mMaterialIndex) == materials.end())
		{
			materials[assimpScene->mMeshes[i]->mMaterialIndex] = CreateMaterial(filepath, assimpScene->mMaterials[assimpScene->mMeshes[i]->mMaterialIndex]);
			G_INFO("Material {} parsed", assimpScene->mMeshes[i]->mMaterialIndex);
		}
		mesh.materialID = materials[assimpScene->mMeshes[i]->mMaterialIndex].id;
	}
	
	G_INFO("Parsing successful. Begin formatting...");
	WriteModelFile(result, materials, outputFile);
	G_INFO("Writing complete. Exiting.");
}
