#include "FrameDecoder.h"

#include "RenderCommands.h"
#include "Renderer.h"
#include "RenderResources.h"
#include "memory/BinaryReader.h"
#include "memory/LinearAllocator.h"

using namespace graphics;
using namespace gold;

static TextureDescription2D ReadCreateTexture2D(BinaryReader& reader)
{
	TextureDescription2D desc;
	desc.mNameHash = reader.Read<u32>();
	desc.mWidth = reader.Read<u32>();
	desc.mHeight = reader.Read<u32>();
	desc.mDataSize = reader.Read<u32>();
	if (desc.mDataSize > 0)
	{
		Memory mem = reader.Read<Memory>();
		desc.mData = mem.data;
	}
	desc.mFormat = reader.Read<TextureFormat>();
	desc.mWrap = reader.Read<TextureWrap>();
	desc.mFilter = reader.Read<TextureFilter>();
	desc.mMipmaps = reader.Read<bool>();
	desc.mBorderColor = reader.Read<glm::vec4>();

	return desc;
}

static RenderState ReadRenderState(BinaryReader& reader, ServerResources& resources)
{
	RenderState state;

	// uniform buffers
	state.mNumUniformBlocks = reader.Read<u32>();
	for (u32 i = 0; i < state.mNumUniformBlocks; ++i)
	{
		RenderState::UniformBlock& buffer = state.mUniformBlocks[i];
		buffer.mNameHash = reader.Read<u32>();
		buffer.mBinding = resources.get(reader.Read<UniformBufferHandle>());
	}

	// shader buffers
	state.mNumStorageBlocks = reader.Read<u32>();
	for (u32 i = 0; i < state.mNumStorageBlocks; ++i)
	{
		RenderState::StorageBlock& buffer = state.mStorageBlocks[i];
		buffer.mNameHash = reader.Read<u32>();
		buffer.mBinding = resources.get(reader.Read<ShaderBufferHandle>());
	}

	// Textures
	state.mNumTextures = reader.Read<u32>();
	for (u32 i = 0; i < state.mNumTextures; ++i)
	{
		RenderState::Texture& texture = state.mTextures[i];
		texture.mNameHash = reader.Read<u32>();
		texture.mHandle = resources.get(reader.Read<TextureHandle>());
	}

	// Images
	state.mNumImages = reader.Read<u32>();
	for (u32 i = 0; i < state.mNumImages; ++i)
	{
		RenderState::Image& image = state.mImages[i];
		image.mNameHash = reader.Read<u32>();
		image.mHandle = resources.get(reader.Read<TextureHandle>());

		// TODO (danielg): Also defined in the frame encoder. Move to shared location
		u8 readBit = image.read ? 1 << 0 : 0;
		u8 writeBit = image.write ? 1 << 1 : 0;

		u8 readWrite = reader.Read<u8>();

		image.read = (readWrite & readBit) > 0;
		image.write = (readWrite & writeBit) > 0;
	}

	// Render pass 
	state.mRenderPass = reader.Read<u8>();

	// Shader
	state.mShader = resources.get(reader.Read<ShaderHandle>());

	// viewport
	state.mViewport.x = reader.Read<int>();
	state.mViewport.y = reader.Read<int>();
	state.mViewport.width = reader.Read<int>();
	state.mViewport.height = reader.Read<int>();

	// Depth Func
	state.mDepthFunc = reader.Read<DepthFunction>();

	// Blend Func
	state.mSrcBlendFunc = reader.Read<BlendFunction>();
	state.mDstBlendFunc = reader.Read<BlendFunction>();

	// CullFace
	state.mCullFace = reader.Read<CullFace>();

	// TODO (danielg): Also defined in the frame encoder. Move to shared location
	// enable/disable booleans
	u8 depthWriteBit = 1 << 0;
	u8 colorWriteBit = 1 << 1;
	u8 alphaBlendBit = 1 << 2;
	u8 wireframeBit = 1 << 3;

	u8 toggles = reader.Read<u8>();

	state.mDepthWriteEnabled = (toggles & depthWriteBit) != 0;
	state.mColorWriteEnabled = (toggles & colorWriteBit) != 0;
	state.mAlphaBlendEnabled = (toggles & alphaBlendBit) != 0;
	state.mWireFrame = (toggles & wireframeBit) != 0;

	return state;
}

void FrameDecoder::Decode(Renderer& renderer, ServerResources& resources, BinaryReader& reader)
{
	std::vector<std::function<void()>> preDrawActions;
	bool complete = false;
	
	auto remap = [&resources](auto clientHandle)
	{
		return clientHandle.idx == 0 ? clientHandle : resources.get(clientHandle);
	};

	auto generatePreDrawFunction = [&preDrawActions]()
	{
		std::function<void()> result = nullptr;

		if (!preDrawActions.empty())
		{
			// NOTE (danielg):	copy is intentional, we need a copied list of the functions
			//					as these calls are deferred
			result = [preDrawActions]()
			{
				for (auto& action : preDrawActions)
				{
					action();
				}
			};
			preDrawActions.clear();
		}
		
		return result;
	};
	
	while (reader.HasData() && !complete)
	{
		RenderCommand command = reader.Read<RenderCommand>();
		switch (command)
		{

			// Uniform Buffers
		case RenderCommand::CreateUniformBuffer:
		{
			UniformBufferHandle clientHandle = reader.Read<UniformBufferHandle>();
			UniformBufferHandle& serverHandle = resources.get(clientHandle);

			Memory mem = reader.Read<Memory>();
			serverHandle = renderer.CreateUniformBuffer(mem.data, mem.size);
			break;
		}
		case RenderCommand::UpdateUniformBuffer:
		{
			UniformBufferHandle serverHandle = resources.get(reader.Read<UniformBufferHandle>());
			
			Memory mem = reader.Read<Memory>();
			u32 offset = reader.Read<u32>();
			preDrawActions.push_back([&renderer, mem, offset, serverHandle]()
			{
				renderer.UpdateUniformBuffer(mem.data, mem.size, offset, serverHandle);
			});

			break;
		}
		case RenderCommand::DestroyUniformBuffer:
		{
			UniformBufferHandle serverHandle = resources.get(reader.Read<UniformBufferHandle>());
			renderer.DestroyUniformBuffer(serverHandle);
			break;
		}

		// Shader Buffers
		case RenderCommand::CreateShaderBuffer:
		{
			ShaderBufferHandle clientHandle = reader.Read<ShaderBufferHandle>();
			ShaderBufferHandle& serverHandle = resources.get(clientHandle);

			Memory mem = reader.Read<Memory>();
			serverHandle = renderer.CreateShaderBuffer(mem.data, mem.size);
			break;
		}
		case RenderCommand::UpdateShaderBuffer:
		{
			ShaderBufferHandle serverHandle = resources.get(reader.Read<ShaderBufferHandle>());
			
			Memory mem = reader.Read<Memory>();
			u32 offset = reader.Read<u32>();
			preDrawActions.push_back([&renderer, mem, offset, serverHandle]()
			{
				renderer.UpdateShaderBuffer(mem.data, mem.size, offset, serverHandle);
			});
			break;
		}
		case RenderCommand::DestroyShaderBuffer:
		{
			ShaderBufferHandle serverHandle = resources.get(reader.Read<ShaderBufferHandle>());
			renderer.DestroyShaderBuffer(serverHandle);
			break;
		}

		// Vertex Buffers
		case RenderCommand::CreateVertexBuffer:
		{
			VertexBufferHandle clientHandle = reader.Read<VertexBufferHandle>();
			VertexBufferHandle& serverHandle = resources.get(clientHandle);

			Memory mem = reader.Read<Memory>();

			serverHandle = renderer.CreateVertexBuffer(mem.data, mem.size);
			break;
		}
		case RenderCommand::UpdateVertexBuffer:
		{
			VertexBufferHandle serverHandle = resources.get(reader.Read<VertexBufferHandle>());
			Memory mem = reader.Read<Memory>();
			u32 offset = reader.Read<u32>();
			preDrawActions.push_back([&renderer, mem, offset, serverHandle]()
			{
				renderer.UpdateVertexBuffer(serverHandle, mem.data, mem.size, offset);
			});
			break;
		}
		case RenderCommand::DestroyVertexBuffer:
		{
			VertexBufferHandle serverHandle = resources.get(reader.Read<VertexBufferHandle>());
			renderer.DestroyVertexBuffer(serverHandle);
			break;
		}

		// Index Buffers
		case RenderCommand::CreateIndexBuffer:
		{
			IndexBufferHandle clientHandle = reader.Read<IndexBufferHandle>();
			IndexBufferHandle& serverHandle = resources.get(clientHandle);

			Memory mem = reader.Read<Memory>();

			serverHandle = renderer.CreateIndexBuffer(mem.data, mem.size);
			break;
		}
		case RenderCommand::UpdateIndexBuffer:
		{
			IndexBufferHandle serverHandle = resources.get(reader.Read<IndexBufferHandle>());
			
			Memory mem = reader.Read<Memory>();
			u32 offset = reader.Read<u32>();
			preDrawActions.push_back([&renderer, mem, offset, serverHandle]()
			{
				renderer.UpdateIndexBuffer(serverHandle, mem.data, mem.size, offset);
			});
			break;
		}
		case RenderCommand::DestroyIndexBuffer:
		{
			IndexBufferHandle serverHandle = resources.get(reader.Read<IndexBufferHandle>());
			renderer.DestroyIndexBuffer(serverHandle);
			break;
		}

		// Shaders
		case RenderCommand::CreateShader:
		{
			ShaderHandle clientHandle = reader.Read<ShaderHandle>();
			
			ShaderSourceDescription desc = reader.Read<ShaderSourceDescription>();

			resources.get(clientHandle) = renderer.CreateShader(desc);
			break;
		}
		case RenderCommand::DestroyShader:
		{
			DEBUG_ASSERT(false, "Not implemented!");
			break;
		}

		// Textures
		case RenderCommand::CreateTexture2D:
		{
			TextureHandle clientHandle = reader.Read<TextureHandle>();
			TextureHandle& serverHandle = resources.get(clientHandle);

			TextureDescription2D desc = ReadCreateTexture2D(reader);
			
			serverHandle = renderer.CreateTexture2D(desc);

			break;
		}
		case RenderCommand::CreateTexture3D:
		{
			TextureHandle& serverHandle = resources.get(reader.Read<TextureHandle>());
			
			TextureDescription3D desc{};
			desc.mWidth = reader.Read<u32>();
			desc.mHeight = reader.Read<u32>();
			desc.mDepth = reader.Read<u32>();

			desc.mDataSize = reader.Read<u32>();
			if (desc.mDataSize > 0)
			{
				desc.mData.reserve(desc.mDepth);
				for (u32 i = 0; i < desc.mDepth; ++i)
				{
					desc.mData[i] = reader.Read<Memory>().data;
				}
			}

			desc.mFormat = reader.Read<TextureFormat>();
			desc.mWrap = reader.Read<TextureWrap>();
			desc.mFilter = reader.Read<TextureFilter>();
			desc.mMipmaps = reader.Read<bool>();
			desc.mBorderColor = reader.Read<glm::vec4>();

			serverHandle = renderer.CreateTexture3D(desc);

			break;
		}
		case RenderCommand::CreateCubemap:
		{
			TextureHandle& serverHandle = resources.get(reader.Read<TextureHandle>());
			
			CubemapDescription desc{};
			desc.mWidth = reader.Read<u32>();
			desc.mHeight = reader.Read<u32>();

			desc.mDataSize = reader.Read<u32>();
			if (desc.mDataSize > 0)
			{
				for (u8 i = 0; i < static_cast<u8>(CubemapFace::COUNT); ++i)
				{
					CubemapFace face = reader.Read<CubemapFace>();
					desc.mData[face] = reader.Read<Memory>().data;
				}
			}

			desc.mFormat = reader.Read<TextureFormat>();
			desc.mWrap = reader.Read<TextureWrap>();
			desc.mFilter = reader.Read<TextureFilter>();

			serverHandle = renderer.CreateCubemap(desc);

			break;
		}
		case RenderCommand::DestroyTexture:
		{
			TextureHandle serverHandle = resources.get(reader.Read<TextureHandle>());
			renderer.DestroyTexture(serverHandle);
			break;
		}
		// Frame Buffers
		case RenderCommand::CreateFrameBuffer:
		{
			FrameBufferHandle clientHandle = reader.Read<FrameBufferHandle>();

			FrameBufferDescription desc;
			constexpr u32 maxAttachments = static_cast<u32>(OutputSlot::Count);
			std::array<TextureHandle, maxAttachments> clientTextures{};
			for (u32 i = 0; i < maxAttachments; ++i)
			{
				clientTextures[i] = reader.Read<TextureHandle>();
				desc.mTextures[i].mDescription = ReadCreateTexture2D(reader);
				desc.mTextures[i].mAttachment = reader.Read<FramebufferAttachment>();
			}

			// client/server handle mapping setup for framebuffer reads
			FrameBuffer fb = renderer.CreateFramebuffer(desc);
			resources.get(clientHandle) = fb.mHandle;
			for (u32 i = 0; i < maxAttachments; ++i)
			{
				if (clientTextures[i].idx)
				{
					resources.get(clientTextures[i]) = fb.mTextures[i];
				}
			}

			break;
		}
		case RenderCommand::DestroyFrameBuffer:
		{
			FrameBufferHandle clientHandle = reader.Read<FrameBufferHandle>();
			FrameBufferHandle serverHandle = resources.get(clientHandle);
			renderer.DestroyFramebuffer(serverHandle);
			resources.Destroy(clientHandle);
			break;
		}

		case RenderCommand::CreateMesh:
		{
			MeshHandle clientHandle = reader.Read<MeshHandle>();
			MeshHandle& serverHandle = resources.get(clientHandle);

			MeshDescription desc;
			desc.mInterlacedBuffer = remap(reader.Read<VertexBufferHandle>());
			desc.mStride = reader.Read<u32>();

			if (desc.mInterlacedBuffer.idx)
			{
				desc.offsets.mPositionOffset = reader.Read<u32>();
				desc.offsets.mNormalsOffset = reader.Read<u32>();
				desc.offsets.mTexCoord0Offset = reader.Read<u32>();
				desc.offsets.mTexCoord1Offset = reader.Read<u32>();
				desc.offsets.mColorsOffset = reader.Read<u32>();
				desc.offsets.mJointsOffset = reader.Read<u32>();
				desc.offsets.mWeightsOffset = reader.Read<u32>();
			}
			else
			{
				desc.handles.mPositions = remap(reader.Read<VertexBufferHandle>());
				desc.handles.mNormals = remap(reader.Read<VertexBufferHandle>());
				desc.handles.mTexCoords0 = remap(reader.Read<VertexBufferHandle>());
				desc.handles.mTexCoords1 = remap(reader.Read<VertexBufferHandle>());
				desc.handles.mColors = remap(reader.Read<VertexBufferHandle>());
				desc.handles.mJoints = remap(reader.Read<VertexBufferHandle>());
				desc.handles.mWeights = remap(reader.Read<VertexBufferHandle>());
			}

			desc.mIndices = remap(reader.Read<IndexBufferHandle>());
			desc.mVertexCount = reader.Read<u32>();
			desc.mIndexCount = reader.Read<u32>();
			desc.mIndexStart = reader.Read<u32>();

			desc.mPrimitiveType = reader.Read <PrimitiveType>();

			desc.mPositionFormat = reader.Read<VertexFormat>();
			desc.mNormalsFormat = reader.Read<VertexFormat>();
			desc.mTexCoord0Format = reader.Read<VertexFormat>();
			desc.mTexCoord1Format = reader.Read<VertexFormat>();
			desc.mColorsFormat = reader.Read<VertexFormat>();
			desc.mJointsFormat = reader.Read<VertexFormat>();
			desc.mWeightsFormat = reader.Read<VertexFormat>();

			desc.mIndicesFormat = reader.Read<IndexFormat>();

			serverHandle = renderer.CreateMesh(desc);

			break;
		}

		// Draw Calls
		case RenderCommand::DrawMesh:
		{
			MeshHandle clientHandle = reader.Read<MeshHandle>();
			MeshHandle serverHandle = resources.get(clientHandle);

			RenderState state = ReadRenderState(reader, resources);

			std::function<void()> preAction = generatePreDrawFunction();
			renderer.DrawMesh(serverHandle, state, preAction);
			break;
		}
		case RenderCommand::DrawMeshInstanced:
		{
			DEBUG_ASSERT(false, "Not implemented!");
			/*
				std::function<void()> preAction = generatePreDrawFunction();
				renderer.DrawMeshInstanced(serverHandle, state, preAction);
			}*/
			break;
		}
		case RenderCommand::DispatchCompute:
		{
			RenderState state = ReadRenderState(reader, resources);
			u16 groupsX = reader.Read<u16>();
			u16 groupsY = reader.Read<u16>();
			u16 groupsZ = reader.Read<u16>();
			
			std::function<void()> preAction = generatePreDrawFunction();
			renderer.DispatchCompute(state, groupsX, groupsY, groupsZ, preAction);
			break;
		}
		// Render Pass
		case RenderCommand::AddRenderPass:
		{
			RenderPass pass;

			Memory name = reader.Read<Memory>();
			pass.mName = (char*)name.data;

			pass.mTarget = remap(reader.Read<FrameBufferHandle>());

			// TODO (danielg): Also defined in the frame encoder. Move to shared location
			// clear color/depth
			u8 clearColorBit = 1 << 0;
			u8 clearDepthBit = 1 << 1;
			u8 clearBits = reader.Read<u8>();

			pass.mClearColor = (clearColorBit & clearBits) == clearColorBit;
			pass.mClearDepth = (clearDepthBit & clearBits) == clearDepthBit;

			pass.mColor = reader.Read<glm::vec4>();
			pass.mDepth = reader.Read<float>();

			renderer.AddRenderPass(pass);

			break;
		}
		case RenderCommand::IssueMemoryBarrier:
		{
			preDrawActions.push_back([&renderer]()
			{
				renderer.IssueMemoryBarrier();
			});
			break;
		}

		case RenderCommand::END:
		{
			complete = true;
			break;
		}
		}
	}	

	for (auto& action : preDrawActions)
	{
		action();
	}
}