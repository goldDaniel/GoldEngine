#include "Renderer.h"

#include <glad/glad.h>

#include <SDL.h>

#include <iostream>
#include <algorithm>

#include <platform/thirdparty/imgui_impl_opengl3.h>
#include <platform/thirdparty/imgui_impl_sdl2.h>

using namespace graphics;

static const GLuint VERTEX_ATTR_POSITION = 0;
static const GLuint VERTEX_ATTR_NORMAL = 1;
static const GLuint VERTEX_ATTR_TEX_COORD0 = 2;
static const GLuint VERTEX_ATTR_TEX_COORD1 = 3;
static const GLuint VERTEX_ATTR_COLOR = 4;
static const GLuint VERTEX_ATTR_JOINTS = 5;
static const GLuint VERTEX_ATTR_WEIGHTS = 6;

static const GLuint VERTEX_ATTR_MODEL_TO_WORLD_COL0 = 7;

static int currentFrame = 0;

static void GLErrorCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, GLchar const* message, const void* user_param)
{
	const char* srcStr = [source]()
	{
		switch (source)
		{
		case GL_DEBUG_SOURCE_API: return "API";
		case GL_DEBUG_SOURCE_WINDOW_SYSTEM: return "WINDOW SYSTEM";
		case GL_DEBUG_SOURCE_SHADER_COMPILER: return "SHADER COMPILER";
		case GL_DEBUG_SOURCE_THIRD_PARTY: return "THIRD PARTY";
		case GL_DEBUG_SOURCE_APPLICATION: return "APPLICATION";
		case GL_DEBUG_SOURCE_OTHER: return "OTHER";
		}
		return "";
	}();

	const char* typeStr = [type]()
	{
		switch (type)
		{
		case GL_DEBUG_TYPE_ERROR:				return "ERROR";
		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: return "DEPRECATED_BEHAVIOR";
		case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:	return "UNDEFINED_BEHAVIOR";
		case GL_DEBUG_TYPE_PORTABILITY:			return "PORTABILITY";
		case GL_DEBUG_TYPE_PERFORMANCE:			return "PERFORMANCE";
		case GL_DEBUG_TYPE_MARKER:				return "MARKER";
		case GL_DEBUG_TYPE_OTHER:				return "OTHER";
		case GL_DEBUG_TYPE_PUSH_GROUP:			return "PUSH GROUP";
		case GL_DEBUG_TYPE_POP_GROUP:			return "POP GROUP";
		}
		return "";;
	}();

	const char* sevStr = [severity]()
	{
		switch (severity)
		{
		case GL_DEBUG_SEVERITY_NOTIFICATION: return "NOTIFICATION";
		case GL_DEBUG_SEVERITY_LOW: return "LOW";
		case GL_DEBUG_SEVERITY_MEDIUM: return "MEDIUM";
		case GL_DEBUG_SEVERITY_HIGH: return "HIGH";
		}

		return "";
	}();

	auto str = "Frame: " + std::to_string(currentFrame) + ": " + srcStr + ", " + typeStr + ", " + sevStr + ", " + std::to_string(id) + ": " + message;

	std::cout << str << std::endl;
}

u8* UniformBuffer::Map()
{
	return static_cast<u8*>(glMapNamedBuffer(mID.idx, GL_WRITE_ONLY));
}

void UniformBuffer::Unmap()
{
	glUnmapNamedBuffer(mID.idx);
}

u8* StorageBuffer::Map()
{
	return static_cast<u8*>(glMapNamedBuffer(mID.idx, GL_WRITE_ONLY));
}

void StorageBuffer::Unmap()
{
	glUnmapNamedBuffer(mID.idx);
}


struct DrawCall
{
	bool isCompute = false;
	u16 groupsX = 0;
	u16 groupsY = 0;
	u16 groupsZ = 0;

	RenderState mState{};
	MeshHandle mMesh{};
	u32 mInstanceCount = 0;
	VertexBufferHandle mInstanceData = { 0 };
	std::function<void()> mPreAction;
};

struct DeleteCommand
{
	enum class Type
	{
		INVALID,

		VertexBuffer,
		IndexBuffer,
		Texture,
		Mesh,
		UniformBuffer,
	};

	Type mType = Type::INVALID;

	union
	{
		MeshHandle mMesh{0};
		VertexBufferHandle mVertexBuffer;
		IndexBufferHandle mIndexBuffer;
		TextureHandle mTexture;
		UniformBufferHandle mUniformBuffer;
	};
};

enum class TextureType
{
	Texture2D,
	Texture3D,
	Cubemap,

	INVALID,
};

struct TextureDesc
{
	TextureType mType = TextureType::INVALID;

	TextureDescription2D desc2D;
	TextureDescription3D desc3D;
	CubemapDescription descCubemap;
	
	TextureDesc(){}
};

struct StateCache
{
	// previous state
	RenderState prevRenderState{};
	FrameBuffer prevFrameBuffer{};
	Mesh prevMesh;
};

static SDL_Window* sdlWindow;
static SDL_GLContext glContext;

static StateCache stateCache;

static std::unordered_map<ShaderHandle, Shader> shaders;
static std::unordered_map<UniformBufferHandle, UniformBuffer> uniformBuffers;
static std::unordered_map<ShaderBufferHandle, StorageBuffer> shaderBuffers;

static std::unordered_map<MeshHandle, Mesh> meshes;

static std::unordered_map<TextureHandle, TextureDesc> textureDescriptions;

static std::vector<DrawCall> drawCalls{};
static std::vector<RenderPass> renderPasses{};
static std::vector<DeleteCommand> deletions{};

static bool buildingFrame = false;

static glm::ivec2 backBufferSize{ 0,0 };

static int32_t maxUBOSize = 0;

static void GatherShaderImages(GLuint program, Shader& result)
{
	// gather textures
	GLint uniformCount;
	glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &uniformCount);
	u64 textureSlot = 0;
	u64 imageSlot = 0;
	for (GLint i = 0; i < uniformCount; ++i)
	{
		char uniformName[128];
		GLint size;
		GLint nameLength;
		GLenum type;
		glGetActiveUniform(program, i, sizeof(uniformName), &nameLength, &size, &type, uniformName);

		GLint loc = glGetUniformLocation(program, uniformName);
		if (type == GL_IMAGE_2D ||
			type == GL_IMAGE_CUBE ||
			type == GL_IMAGE_3D)
		{
			glProgramUniform1i(program, loc, static_cast<GLint>(imageSlot));

			result.mImages[imageSlot] = util::Hash(uniformName, nameLength);
			imageSlot++;
		}
	}
}

static void GatherShaderTextures(GLuint program, Shader& result)
{
	// gather textures
	GLint uniformCount;
	glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &uniformCount);
	u64 textureSlot = 0;
	for (GLint i = 0; i < uniformCount; ++i)
	{
		char uniformName[128];
		GLint size;
		GLint nameLength;
		GLenum type;
		glGetActiveUniform(program, i, sizeof(uniformName), &nameLength, &size, &type, uniformName);

		GLint loc = glGetUniformLocation(program, uniformName);

		if (type == GL_SAMPLER_2D ||
			type == GL_SAMPLER_CUBE ||
			type == GL_SAMPLER_3D)
		{
			glProgramUniform1i(program, loc, static_cast<GLint>(textureSlot));

			result.mTextures[textureSlot] = util::Hash(uniformName, nameLength);
			textureSlot++;
		}
	}
}

static void GatherShaderUniformBlocks(GLuint program, Shader& result)
{
	// gather uniform blocks
	GLint numUBOs;
	GLint maxNameLen;
	glGetProgramInterfaceiv(program, GL_UNIFORM_BLOCK, GL_ACTIVE_RESOURCES, &numUBOs);
	glGetProgramInterfaceiv(program, GL_UNIFORM_BLOCK, GL_MAX_NAME_LENGTH, &maxNameLen);
	std::vector<char> name(maxNameLen);

	for (int i = 0; i < numUBOs; ++i)
	{
		GLsizei nameLen;
		glGetProgramResourceName(program, GL_UNIFORM_BLOCK, i, maxNameLen, &nameLen, name.data());
		result.mUniformBlocks[i] = util::Hash(&name[0], nameLen);
		glUniformBlockBinding(program, i, i);
	}
}

static void GatherShaderStorageBlocks(GLuint program, Shader& result)
{
	// gather storage blocks
	GLint numSSBOs;
	GLint maxNameLen;
	glGetProgramInterfaceiv(program, GL_SHADER_STORAGE_BLOCK, GL_ACTIVE_RESOURCES, &numSSBOs);
	glGetProgramInterfaceiv(program, GL_SHADER_STORAGE_BLOCK, GL_MAX_NAME_LENGTH, &maxNameLen);
	std::vector<char> name(maxNameLen);

	for (int i = 0; i < numSSBOs; ++i)
	{
		GLsizei nameLen;
		glGetProgramResourceName(program, GL_SHADER_STORAGE_BLOCK, i, maxNameLen, &nameLen, name.data());
		result.mStorageBlocks[i] = util::Hash(&name[0], nameLen);
		glShaderStorageBlockBinding(program, i, i);
	}
}

static auto FilterToGL(TextureFilter filter, GLenum& minFilter, GLenum& magFilter, bool mipmaps)
{
	switch (filter)
	{
	case TextureFilter::LINEAR:
		minFilter = mipmaps ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR;
		magFilter = GL_LINEAR;
		return;

	case TextureFilter::POINT:
		minFilter = mipmaps ? GL_NEAREST_MIPMAP_NEAREST : GL_NEAREST;
		magFilter = GL_NEAREST;
		return;
	}

	DEBUG_ASSERT(false, "Unsupported filter!");
}

static auto WrapToGL(TextureWrap wrap)
{
	switch (wrap)
	{
	case graphics::TextureWrap::REPEAT: return GL_REPEAT;
	case graphics::TextureWrap::CLAMP:  return GL_CLAMP_TO_EDGE;
	case graphics::TextureWrap::MIRROR: return GL_MIRRORED_REPEAT;
	case graphics::TextureWrap::BORDER: return GL_CLAMP_TO_BORDER;
	}

	DEBUG_ASSERT(false, "Invalid TextureWrap");
	return GL_INVALID_ENUM;
};

static auto ToGlBlendFunc(BlendFunction func)
{
	switch (func)
	{
	case BlendFunction::SRC_ALPHA: return GL_SRC_ALPHA;
	case BlendFunction::ONE_MINUS_SRC_ALPHA: return GL_ONE_MINUS_SRC_ALPHA;
	case BlendFunction::ONE: return GL_ONE;
	case BlendFunction::ZERO: return GL_ZERO;
	}

	DEBUG_ASSERT(false, "Invalid Depth Function!");
	return GL_INVALID_ENUM;
};

static auto TypeToGL(TextureFormat format, GLenum& channels, GLenum& type)
{
	switch (format) 
	{
	case TextureFormat::R_U8:
	case TextureFormat::R_U8NORM:
		channels = GL_RED;
		type = GL_UNSIGNED_BYTE;
		break;
	case TextureFormat::R_U16:
		channels = GL_RED;
		type = GL_UNSIGNED_SHORT;
		break;
	case TextureFormat::R_FLOAT:
		channels = GL_RED;
		type = GL_FLOAT;
		break;

	case TextureFormat::RGB_U8:
	case TextureFormat::RGB_U8_SRGB:
		channels = GL_RGB;
		type = GL_UNSIGNED_BYTE;
		break;
	case TextureFormat::RGB_HALF:
		channels = GL_RGB;
		type = GL_HALF_FLOAT;
		break;
	case TextureFormat::RGB_FLOAT:
		channels = GL_RGB;
		type = GL_FLOAT;
		break;

	case TextureFormat::RGBA_U8:
	case TextureFormat::RGBA_U8_SRGB:
		channels = GL_RGBA;
		type = GL_UNSIGNED_BYTE;
		break;
	case TextureFormat::RGBA_HALF:
		channels = GL_RGBA;
		type = GL_HALF_FLOAT;
		break;
	case TextureFormat::RGBA_FLOAT:
		channels = GL_RGBA;
		type = GL_FLOAT;
		break;

	case TextureFormat::DEPTH:
		channels = GL_DEPTH_COMPONENT;
		type = GL_FLOAT;
		break;

	default:
		DEBUG_ASSERT(false, "channel/type selection invalid!");
		break;

	}
};

static auto FormatToInternalGL(TextureFormat format)
{
	switch (format)
	{
	case graphics::TextureFormat::INVALID:		break;

	case graphics::TextureFormat::R_U8:			return GL_R8;
	case graphics::TextureFormat::R_U8NORM:		return GL_R8_SNORM;
	case graphics::TextureFormat::R_U16:		return GL_R16;
	case graphics::TextureFormat::R_FLOAT:		return GL_R32F;

	case graphics::TextureFormat::RGB_U8:		return GL_RGB8;
	case graphics::TextureFormat::RGB_U8_SRGB:	return GL_SRGB8;
	case graphics::TextureFormat::RGB_HALF:		return GL_RGB16F;
	case graphics::TextureFormat::RGB_FLOAT:	return GL_RGB32F;

	case graphics::TextureFormat::RGBA_U8:		return GL_RGBA8;
	case graphics::TextureFormat::RGBA_U8_SRGB:	return GL_SRGB8_ALPHA8;

	case graphics::TextureFormat::RGBA_HALF:	return GL_RGBA16F;
	case graphics::TextureFormat::RGBA_FLOAT:	return GL_RGBA32F;

	case graphics::TextureFormat::DEPTH:		return GL_DEPTH_COMPONENT24;
	}

	DEBUG_ASSERT(false, "Unsupported Texture Format");
	return GL_INVALID_ENUM;
};


static u32 CreateGLBuffer(const void* data, u64 size, BufferUsage usage)
{
	DEBUG_ASSERT(size < std::numeric_limits<u32>::max(), "Invalid buffer size!");

	auto bufferUsageGL = [](BufferUsage usage)
	{
		switch (usage)
		{
		case graphics::BufferUsage::STATIC:		return GL_STATIC_DRAW;
		case graphics::BufferUsage::DYNAMIC:	return GL_DYNAMIC_DRAW;
		case graphics::BufferUsage::STREAM:		return GL_STREAM_DRAW;
		}

		DEBUG_ASSERT(false, "Invalid buffer usage!");
		return GL_INVALID_ENUM;
	};


	u32 handle = 0;
	glCreateBuffers(1, &handle);
	glNamedBufferData(handle, size, data, bufferUsageGL(usage));

	// NOTE (danielg): some drivers don't zero out buffer memory on creation
	if (!data)
	{
		u8* ptr = (u8*)glMapNamedBuffer(handle, GL_WRITE_ONLY);
		memset(ptr, 0, size);
		glUnmapNamedBuffer(handle);
	}

	return handle;
}

static void UpdateGLBuffer(u32 handle, const void* data, u64 offset, u64 size)
{
	DEBUG_ASSERT(size < std::numeric_limits<int32_t>::max(), "Buffer allocation too large");
	glNamedBufferSubData(handle, offset, size, data);
}

void Renderer::SetBackBufferSize(int w, int h)
{
	backBufferSize = { w, h };
}

Renderer::~Renderer()
{
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();

	SDL_GL_DeleteContext(glContext);
}

void Renderer::Init(void* window)
{
	G_ENGINE_INFO("Renderer Initializing...");

	sdlWindow = (SDL_Window*)window;

	SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);

	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

	SDL_GL_SetSwapInterval(1);

	glContext = SDL_GL_CreateContext(sdlWindow);
	if (!glContext || !gladLoadGLLoader(SDL_GL_GetProcAddress))
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Failed to Initialize Window!", "failed to initialize OpenGL Context", 0);
		SDL_Log("Failed to initialize the OpenGL context.");
		exit(1);
	}

	ImGui_ImplSDL2_InitForOpenGL(sdlWindow, glContext);
	ImGui_ImplOpenGL3_Init("#version 330");
	
	G_ENGINE_WARN("OpenGL Loaded:");
	G_ENGINE_TRACE("Vendor: {}", (const char*)glGetString(GL_VENDOR));
	G_ENGINE_TRACE("Renderer: {}", (const char*)glGetString(GL_RENDERER));
	G_ENGINE_TRACE("Version: {}", (const char*)glGetString(GL_VERSION));



	glEnable(GL_DEBUG_OUTPUT);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	glDebugMessageCallback(GLErrorCallback, nullptr);
	glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_FALSE);

	glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &maxUBOSize);
	
	buildingFrame = false;
	stateCache.prevMesh = {};
	 
	// default GL state
	// done using prevState so if we want to change defaults we just need
	// to change the RenderState struct defaults
	glDepthMask(stateCache.prevRenderState.mDepthWriteEnabled ? GL_TRUE : GL_FALSE);
	if (stateCache.prevRenderState.mCullFace == CullFace::DISABLED)
	{
		glDisable(GL_CULL_FACE);
	}
	else
	{
		glEnable(GL_CULL_FACE);
	}
	glCullFace(stateCache.prevRenderState.mCullFace == CullFace::FRONT ? GL_FRONT : GL_BACK);

	if (stateCache.prevRenderState.mDepthFunc == DepthFunction::DISABLED)
	{
		glDisable(GL_DEPTH_TEST);
	}
	else
	{
		glEnable(GL_DEPTH_TEST);
		
		glDepthFunc(GL_LESS);
	}
	
	
	glBlendFunc(ToGlBlendFunc(stateCache.prevRenderState.mSrcBlendFunc), ToGlBlendFunc(stateCache.prevRenderState.mDstBlendFunc));
	if (stateCache.prevRenderState.mAlphaBlendEnabled)
	{
		glEnable(GL_BLEND);
	}
	else
	{
		glDisable(GL_BLEND);
	}

	if (stateCache.prevRenderState.mWireFrame)
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glEnable(GL_POLYGON_OFFSET_LINE);
		glPolygonOffset(-1, -1);
	}
	else
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glDisable(GL_POLYGON_OFFSET_LINE);
	}
}

void Renderer::Destroy()
{

}

void Renderer::BeginFrame()
{
	DEBUG_ASSERT(!buildingFrame, "Cannot start new frame when one is in flight!");
	buildingFrame = true;
	
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplSDL2_NewFrame(sdlWindow);
	ImGui::NewFrame();

	drawCalls.clear();
	renderPasses.clear();
	deletions.clear();

	// NOTE (danielg):	We reset the state at the start of the frame 
	//					because we use the default ImGui GL implementation
	//					which modifies state external to this backend. 
	//					TODO: Implement ImGui rendering using our renderer
	stateCache = {};
}

void Renderer::ClearBackBuffer()
{
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, stateCache.prevFrameBuffer.mHandle);
}

void Renderer::EndFrame()
{
	DEBUG_ASSERT(buildingFrame, "Cannot end frame if one is not building!");
	buildingFrame = false;

	auto setRenderState = [](const RenderState& state, bool isCompute)
	{
		static auto depthFuncGL = [](DepthFunction func)
		{
			switch (func)
			{
			case DepthFunction::LESS: return GL_LESS;
			case DepthFunction::LESS_EQUAL: return GL_LEQUAL;
			case DepthFunction::EQUAL: return GL_EQUAL;
			case DepthFunction::ALWAYS: return GL_ALWAYS;
			case DepthFunction::DISABLED: return GL_ALWAYS;
			}

			DEBUG_ASSERT(false, "Invalid Depth Function!");
			return GL_INVALID_ENUM;
		};

		if (state.mShader.idx != stateCache.prevRenderState.mShader.idx)
		{
			glUseProgram(state.mShader.idx);
		}


		const Shader& shader = shaders[state.mShader];

		// uniforms blocks
		for (u64 i = 0; i < shader.mUniformBlocks.size(); ++i)
		{
			bool foundBinding = false;

			for (u64 j = 0; j < state.mNumUniformBlocks; ++j)
			{
				UniformBufferHandle binding = state.mUniformBlocks[j].mBinding;
				const UniformBuffer& buffer = uniformBuffers[binding];

				u32 nameHash = state.mUniformBlocks[j].mNameHash;
				if (nameHash == shader.mUniformBlocks[i] && buffer.mSize > 0)
				{
					glBindBufferRange(GL_UNIFORM_BUFFER, static_cast<GLuint>(i), binding.idx, 0, buffer.mSize);
					foundBinding = true;
					break;		
				}
			
			}
			if (!foundBinding)
			{
				glBindBufferBase(GL_UNIFORM_BUFFER, static_cast<GLuint>(i), 0);
			}
		}

		// shader storage blocks
		for (u64 i = 0; i < shader.mStorageBlocks.size(); ++i)
		{
			bool foundBinding = false;

			for (u64 j = 0; j < state.mNumStorageBlocks; ++j)
			{
				ShaderBufferHandle binding = state.mStorageBlocks[j].mBinding;
				const StorageBuffer& buffer = shaderBuffers[binding];


				u32 nameHash = state.mStorageBlocks[j].mNameHash;
				if (nameHash == shader.mStorageBlocks[i] && buffer.mSize > 0)
				{
					glBindBufferRange(GL_SHADER_STORAGE_BUFFER, static_cast<GLuint>(i), binding.idx, 0, buffer.mSize);
					foundBinding = true;
					break;
				}
			}
			if (!foundBinding)
			{
				glBindBufferBase(GL_SHADER_STORAGE_BUFFER, static_cast<GLuint>(i), 0);
			}
		}

		// textures
		for (u64 i = 0; i < shader.mTextures.size(); ++i)
		{
			bool foundTexture = false;
			for (u64 j = 0; j < state.mNumTextures; ++j)
			{
				u32 nameHash = state.mTextures[j].mNameHash;
				if (nameHash == shader.mTextures[i])
				{
					TextureHandle handle = state.mTextures[j].mHandle;
					glBindTextureUnit(static_cast<GLuint>(i), handle.idx);

					foundTexture = true;
					break;
				}
			}
		}

		// Images
		if (isCompute)
		{
			for (u64 i = 0; i < shader.mImages.size(); ++i)
			{
				bool foundTexture = false;
				for (u64 j = 0; j < state.mNumImages; ++j)
				{
					u32 nameHash = state.mImages[j].mNameHash;
					if (nameHash == shader.mImages[i])
					{
						TextureHandle handle = state.mImages[j].mHandle;
						const TextureDesc& d = textureDescriptions[handle];

						GLenum format;
						switch (d.mType)
						{
						case TextureType::Cubemap: 
							format = FormatToInternalGL(d.descCubemap.mFormat);
							break;
						case TextureType::Texture2D:
							format = FormatToInternalGL(d.desc2D.mFormat);
							break;
						case TextureType::Texture3D:
							format = FormatToInternalGL(d.desc3D.mFormat);
							break;
						default:
							DEBUG_ASSERT(false, "Invalid texture type");
							break;
						}

						GLenum access = 0;
						if (state.mImages[j].read && state.mImages[j].write)
						{
							access = GL_READ_WRITE;
						}
						else if (state.mImages[j].read)
						{
							access = GL_READ_ONLY;
						}
						else if (state.mImages[j].write)
						{
							access = GL_WRITE_ONLY;
						}
						else 
						{
							DEBUG_ASSERT(false, "Invalid access type");
						}

						glBindImageTexture(static_cast<GLuint>(i), handle.idx, 0, GL_TRUE, 0, access, format);
						foundTexture = true;
						break;
					}
				}
			}
		}		

		// depth
		if (state.mDepthWriteEnabled != stateCache.prevRenderState.mDepthWriteEnabled)
		{
			glDepthMask(state.mDepthWriteEnabled ? GL_TRUE : GL_FALSE);
		}

		// face culling 
		if (state.mCullFace != stateCache.prevRenderState.mCullFace)
		{
			if (state.mCullFace == CullFace::DISABLED)
			{
				glDisable(GL_CULL_FACE);
			}
			else
			{
				if (stateCache.prevRenderState.mCullFace == CullFace::DISABLED)
				{
					glEnable(GL_CULL_FACE);
				}
				glCullFace(state.mCullFace == CullFace::FRONT ? GL_FRONT : GL_BACK);
			}
		}

		// depth test
		if (state.mDepthFunc != stateCache.prevRenderState.mDepthFunc)
		{
			if (state.mDepthFunc == DepthFunction::DISABLED)
			{
				glDisable(GL_DEPTH_TEST);
			}
			else
			{
				if (stateCache.prevRenderState.mDepthFunc == DepthFunction::DISABLED)
				{
					glEnable(GL_DEPTH_TEST);
				}
				glDepthFunc(depthFuncGL(state.mDepthFunc));
			}
		}

		// alpha blend
		if (stateCache.prevRenderState.mSrcBlendFunc != state.mSrcBlendFunc || stateCache.prevRenderState.mDstBlendFunc != state.mDstBlendFunc)
		{
			glBlendFunc(ToGlBlendFunc(state.mSrcBlendFunc), ToGlBlendFunc(state.mDstBlendFunc));
		}
		if (state.mAlphaBlendEnabled != stateCache.prevRenderState.mAlphaBlendEnabled)
		{
			if (state.mAlphaBlendEnabled)
			{
				glEnable(GL_BLEND);		
			}
			else
			{
				glDisable(GL_BLEND);
			}
		}

		// polyfill mode
		if (state.mWireFrame != stateCache.prevRenderState.mWireFrame)
		{
			if (state.mWireFrame)
			{
				glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
				glEnable(GL_POLYGON_OFFSET_LINE);
				glPolygonOffset(-1, -1);
			}
			else
			{
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
				glDisable(GL_POLYGON_OFFSET_LINE);
			}
		}

		if (stateCache.prevRenderState.mViewport != state.mViewport)
		{
			// NOTE (danielg): if we do not set the viewport manually, we want it to be the width/height of the framebuffer
			if( (state.mViewport.x     == 0 && state.mViewport.y      == 0 && 
				 state.mViewport.width == 0 && state.mViewport.height == 0))
			{
				const auto& renderPass = renderPasses[state.mRenderPass];
				const auto& frameBuffer = renderPass.mTarget;

				glViewport(0, 0, frameBuffer.mWidth, frameBuffer.mHeight);
			}
			else
			{
				glViewport(state.mViewport.x, state.mViewport.y, state.mViewport.width, state.mViewport.height);
			}
		}

		stateCache.prevRenderState = state;
	};

	auto setupRenderPass = [](const RenderPass& pass)
	{
		glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, pass.mName);

		const FrameBuffer& framebuffer = pass.mTarget;

		if (framebuffer.mHandle != stateCache.prevFrameBuffer.mHandle)
		{
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer.mHandle);
			stateCache.prevFrameBuffer.mHandle = framebuffer.mHandle;
		}

		GLbitfield mask = 0 | (pass.mClearColor ? GL_COLOR_BUFFER_BIT : 0)
							| (pass.mClearDepth ? GL_DEPTH_BUFFER_BIT : 0);

		if (mask)
		{
			// FrameBufferHandle == 0 is the backbuffer
			glm::ivec4 mViewport = { 0, 0, backBufferSize.x, backBufferSize.y };
			if (framebuffer.mHandle)
			{
				mViewport = {0, 0, framebuffer.mWidth, framebuffer.mHeight };
			}

			glm::ivec4 prevViewport = 
			{ 
				stateCache.prevRenderState.mViewport.x, 
				stateCache.prevRenderState.mViewport.y, 
				stateCache.prevRenderState.mViewport.width, 
				stateCache.prevRenderState.mViewport.height 
			};

			if (mViewport != prevViewport)
			{
				glViewport(0, 0, static_cast<GLsizei>(mViewport.z), static_cast<GLsizei>(mViewport.w));
			}
			
			if (pass.mClearColor)
			{
				glClearColor(pass.mColor.r, pass.mColor.g, pass.mColor.b, pass.mColor.a);
			}
			if (pass.mClearDepth)
			{
				glClearDepth(pass.mDepth);
			}
			glClear(mask);
		}
	};

	auto primitiveTypeGL = [](PrimitiveType prim)
	{
		switch (prim)
		{
		case PrimitiveType::TRIANGLES: return GL_TRIANGLES;
		case PrimitiveType::TRIANGLE_STRIP: return GL_TRIANGLE_STRIP;
		case PrimitiveType::POINTS: return GL_POINTS;
		case PrimitiveType::LINES: return GL_LINES;
		}

		DEBUG_ASSERT(false, "invalid primitive type!");
		return GL_INVALID_ENUM;
	};

	auto drawCallSingle = [&](const DrawCall& draw, GLenum primitive)
	{
		const Mesh& mesh = meshes[draw.mMesh];
		
		if (mesh.mID != stateCache.prevMesh.mID)
		{
			glBindVertexArray(mesh.mID);
		}
		

		if (mesh.mIndices.idx)
		{
			GLenum indexFormat = mesh.mIndexFormat == IndexFormat::U16 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT;
			glDrawElementsBaseVertex(primitive, mesh.mIndexCount, indexFormat, 0, static_cast<GLint>(mesh.mIndexStart));
		}
		else
		{
			glDrawArrays(primitive, 0, mesh.mVertexCount);
		}

		stateCache.prevMesh = mesh;
	};

	
	auto drawCallInstanced = [&](const DrawCall& draw, GLenum primitive)
	{
		const Mesh& mesh = meshes[draw.mMesh];

		if (mesh.mID != stateCache.prevMesh.mID) {
			glBindVertexArray(mesh.mID);
		}
		
		//note (Danielg): draw instanced (ONLY SUPPORTS MATRICES)
		if (draw.mInstanceData.idx)
		{
			glBindBuffer(GL_ARRAY_BUFFER, draw.mInstanceData.idx);
			for (int i = 0; i < 4; ++i)
			{
				GLuint loc = VERTEX_ATTR_MODEL_TO_WORLD_COL0 + i;

				glEnableVertexAttribArray(loc);

				GLsizei stride = sizeof(glm::mat4);
				void* offset = (void*)(i * sizeof(glm::vec4));

				glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, stride, offset);
				glVertexAttribDivisor(loc, 1);
			}
		}

		if (mesh.mIndices.idx)
		{
			glDrawElementsInstanced(primitive, mesh.mIndexCount, GL_UNSIGNED_INT, 0, static_cast<GLsizei>(draw.mInstanceCount));
		}
		else
		{
			glDrawArraysInstanced(primitive, 0, mesh.mVertexCount, static_cast<GLsizei>(draw.mInstanceCount));
		}
	};

	auto setPatchParameters = [](GLenum primitive)
	{
		if (primitive == GL_TRIANGLES || primitive == GL_TRIANGLE_STRIP)
		{
			glPatchParameteri(GL_PATCH_VERTICES, 3);
		}
		else if (primitive == GL_LINES || GL_LINE_STRIP)
		{
			glPatchParameteri(GL_PATCH_VERTICES, 2);
		}
	};
	
	std::sort(drawCalls.begin(), drawCalls.end(), [](const DrawCall& a, const DrawCall& b)
	{
		if (a.mState.mRenderPass != b.mState.mRenderPass)
		{
			return a.mState.mRenderPass < b.mState.mRenderPass;
		}
		return a.mState.mShader.idx < b.mState.mShader.idx;
	});

	u8 passIndex = std::numeric_limits<u8>::max();
	for (auto& draw : drawCalls) 
	{
		if (draw.mState.mRenderPass != passIndex)
		{
			if (passIndex != std::numeric_limits<u8>::max())
			{
				glPopDebugGroup();
			}
			
			setupRenderPass(renderPasses[draw.mState.mRenderPass]);
			passIndex = draw.mState.mRenderPass;
		}

		if (draw.mPreAction)
		{
			draw.mPreAction(); 
		}

		setRenderState(draw.mState, draw.isCompute);
		
		if (draw.isCompute)
		{
			DEBUG_ASSERT(draw.groupsX > 0, "Invalid thread group count!");
			DEBUG_ASSERT(draw.groupsY > 0, "Invalid thread group count!");
			DEBUG_ASSERT(draw.groupsZ > 0, "Invalid thread group count!");
			
			//TODO (danielg): support explicit memory barriers so we aren't always waiting
			glDispatchCompute(draw.groupsX, draw.groupsY, draw.groupsZ);
			glMemoryBarrier(GL_ALL_BARRIER_BITS);
		}
		else
		{
			Mesh& mesh = meshes[draw.mMesh];
			Shader& shader = shaders[draw.mState.mShader];
			GLenum prim = primitiveTypeGL(mesh.mPrimitiveType);
			if (shader.mTesselation)
			{
				setPatchParameters(prim);
				prim = GL_PATCHES;
			}
			
			if (draw.mInstanceCount)
			{
				drawCallInstanced(draw, prim);
			}
			else
			{
				drawCallSingle(draw, prim);
			}
		}
	}
	
	if (passIndex != std::numeric_limits<u8>::max())
	{
		glPopDebugGroup();
	}

	// resource deletion
	for (const auto& del : deletions)
	{
		switch (del.mType)
		{
		case DeleteCommand::Type::VertexBuffer:
			glDeleteBuffers(1, &del.mVertexBuffer.idx);
			break;
		case DeleteCommand::Type::IndexBuffer:
			glDeleteBuffers(1, &del.mIndexBuffer.idx);
			break;
		case DeleteCommand::Type::UniformBuffer: 
			glDeleteBuffers(1, &del.mUniformBuffer.idx);
			break;
		case DeleteCommand::Type::Texture:
			glDeleteTextures(1, &del.mTexture.idx);
			break;
		case DeleteCommand::Type::Mesh:
			Mesh& mesh = meshes[del.mMesh];
			if (mesh.mPositions.idx)	glDeleteBuffers(1, &mesh.mPositions.idx);
			if (mesh.mNormals.idx)		glDeleteBuffers(1, &mesh.mNormals.idx);
			if (mesh.mTexCoords0.idx)	glDeleteBuffers(1, &mesh.mTexCoords0.idx);
			if (mesh.mTexCoords1.idx)	glDeleteBuffers(1, &mesh.mTexCoords1.idx);
			if (mesh.mColors.idx)		glDeleteBuffers(1, &mesh.mColors.idx);
			if (mesh.mJoints.idx)		glDeleteBuffers(1, &mesh.mJoints.idx);
			if (mesh.mWeights.idx)		glDeleteBuffers(1, &mesh.mWeights.idx);
			if (mesh.mIndices.idx)		glDeleteBuffers(1, &mesh.mIndices.idx);

			MeshHandle handle = { mesh.mID };
			glDeleteVertexArrays(1, &mesh.mID);
			meshes.erase(handle);
			break;
		}
	}

	currentFrame++;

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	SDL_GL_SwapWindow(sdlWindow);
}

u8 Renderer::AddRenderPass(const RenderPass& desc)
{
	renderPasses.push_back(desc);
	return static_cast<u8>(renderPasses.size() - 1);
}

u8 Renderer::AddRenderPass(const char* name, FrameBuffer target, ClearColor color, ClearDepth depth)
{
	RenderPass pass{};
	pass.mName = name;
	pass.mTarget = target;
	pass.mClearColor = color == ClearColor::YES;
	pass.mClearDepth = depth == ClearDepth::YES;
	
	return AddRenderPass(pass);
}

u8 Renderer::AddRenderPass(const char* name, ClearColor color, ClearDepth depth)
{
	FrameBuffer framebufffer{};
	return AddRenderPass(name, framebufffer, color, depth);
}

ShaderBufferHandle Renderer::CreateStorageBlock(const void* data, u32 size)
{
	StorageBuffer result;

	ShaderBufferHandle handle = { CreateGLBuffer(data, size, BufferUsage::DYNAMIC) };
	shaderBuffers[handle] = { handle, size };

	return handle;
}

UniformBufferHandle Renderer::CreateUniformBlock(const void* data, u32 size)
{
	DEBUG_ASSERT(size < static_cast<u32>(maxUBOSize), "Size larger than max UBO size!");

	UniformBufferHandle handle = { CreateGLBuffer(data, size, BufferUsage::DYNAMIC) };
	uniformBuffers[handle] = { handle, size };

	return handle;
}

void Renderer::UpdateStorageBlock(const void* data, u32 size, u32 offset, ShaderBufferHandle binding)
{
	//DEBUG_ASSERT(size <= binding.mSize, "Size is larger than storage block size!");
	UpdateGLBuffer(binding.idx, data, offset, size);
}

void Renderer::UpdateUniformBlock(const void* data, u32 size, u32 offset, UniformBufferHandle binding)
{
	//DEBUG_ASSERT(size <= binding.mSize, "Size is larger than storage block size!");
	UpdateGLBuffer(binding.idx, data, offset, size);
}

void graphics::Renderer::DestroyUniformBlock(UniformBufferHandle handle)
{
	DeleteCommand command{};
	command.mType = DeleteCommand::Type::UniformBuffer;
	command.mUniformBuffer = handle;

	deletions.push_back(command);
}


VertexBufferHandle Renderer::CreateVertexBuffer(const void* data, u32 size, BufferUsage usage)
{
	return { CreateGLBuffer(data, size, usage) };
}

void Renderer::UpdateVertexBuffer(VertexBufferHandle handle, const void* data, u32 size)
{
	UpdateGLBuffer(handle.idx, data, 0, size);
}

void Renderer::UpdateIndexBuffer(IndexBufferHandle handle, const void* data, u32 size)
{
	UpdateGLBuffer(handle.idx, data, 0, size);
}

void Renderer::DestroyVertexBuffer(VertexBufferHandle handle)
{

	DeleteCommand command{};
	command.mType = DeleteCommand::Type::VertexBuffer;
	command.mVertexBuffer = handle;

	deletions.push_back(command);
}

IndexBufferHandle Renderer::CreateIndexBuffer(const void* data, u32 size, BufferUsage usage)
{
	return { CreateGLBuffer(data, size, usage) };
}

void Renderer::DestroyIndexBuffer(IndexBufferHandle handle)
{
	DeleteCommand command{};
	command.mType = DeleteCommand::Type::IndexBuffer;
	command.mIndexBuffer = handle;

	deletions.push_back(command);
}

TextureHandle Renderer::CreateCubemap(const CubemapDescription& desc)
{
	auto faceToGL = [](CubemapFace face)
	{
		switch (face)
		{
		case CubemapFace::POSITIVE_X: return GL_TEXTURE_CUBE_MAP_POSITIVE_X;
		case CubemapFace::POSITIVE_Y: return GL_TEXTURE_CUBE_MAP_POSITIVE_Y;
		case CubemapFace::POSITIVE_Z: return GL_TEXTURE_CUBE_MAP_POSITIVE_Z;

		case CubemapFace::NEGATIVE_X: return GL_TEXTURE_CUBE_MAP_NEGATIVE_X;
		case CubemapFace::NEGATIVE_Y: return GL_TEXTURE_CUBE_MAP_NEGATIVE_Y;
		case CubemapFace::NEGATIVE_Z: return GL_TEXTURE_CUBE_MAP_NEGATIVE_Z;

		default: 
			DEBUG_ASSERT(false, "Invalid cube map face!"); 
			return GL_INVALID_ENUM;
		}
	};


	GLenum min;
	GLenum mag;
	FilterToGL(desc.mFilter, min, mag, false);


	GLenum channels;
	GLenum type;
	TypeToGL(desc.mFormat, channels, type);

	GLenum internal = FormatToInternalGL(desc.mFormat);

	GLuint texture;
	glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &texture);
	glTextureStorage2D(texture, 1, internal, desc.mWidth, desc.mHeight);
	
	//HACK (danielg): 
	// Current (as of November 2022) Some AMD Drivers have a bug where you cant upload anything but 
	// the positive X face of a cubemap with glTextureSubImage3D! Use 2d texture views to work around this.
	//for (auto& iter : desc.mData)
	//{
	//	auto face = iter.first;
	//	const auto data = iter.second;
	//	glTextureSubImage3D(texture, 0, 0, 0, faceToGL(face), desc.mWidth, desc.mHeight, 1, channels, type, data);
	//}

	std::array<GLuint, 6> faceViews = { };
	glGenTextures(static_cast<GLsizei>(faceViews.size()), &faceViews[0]);

	for (GLint i = 0; i < faceViews.size(); ++i)
	{
		GLuint view = faceViews[i];
		const void* data = desc.mData.at(static_cast<CubemapFace>(i));

		glTextureView(view, GL_TEXTURE_2D, texture, internal, 0, 1, i, 1);
		glTextureSubImage2D(view, 0, 0, 0, desc.mWidth, desc.mHeight, channels, type, data);
	}
	glDeleteTextures(static_cast<GLsizei>(faceViews.size()), &faceViews[0]);

	
	glTextureParameteri(texture, GL_TEXTURE_WRAP_S, WrapToGL(desc.mWrap));
	glTextureParameteri(texture, GL_TEXTURE_WRAP_T, WrapToGL(desc.mWrap));
	glTextureParameteri(texture, GL_TEXTURE_MIN_FILTER, min);
	glTextureParameteri(texture, GL_TEXTURE_MAG_FILTER, mag);

	TextureDesc d;
	d.mType = TextureType::Cubemap;
	d.descCubemap = desc;

	textureDescriptions[{texture}] = std::move(d);

	return { texture };
}

void Renderer::TextureReadback(const TextureHandle handle, u8* buffer, u32 size)
{
	const auto& desc = textureDescriptions[handle];

	GLenum channels = 0;
	GLenum type = 0;
	switch (desc.mType)
	{
	case TextureType::Texture2D:
		TypeToGL(desc.desc2D.mFormat, channels, type);
		break;
	case TextureType::Texture3D:
		TypeToGL(desc.desc3D.mFormat, channels, type);
		break;
	case TextureType::Cubemap:
		TypeToGL(desc.descCubemap.mFormat, channels, type);
		break;
	}

	glGetTextureImage(handle.idx, 0, channels, type, size, buffer);
}

TextureHandle Renderer::CreateTexture2D(const TextureDescription2D& desc)
{
	DEBUG_ASSERT(desc.mFormat != TextureFormat::INVALID, "Invalid texture format!");

	GLenum min;
	GLenum mag;
	FilterToGL(desc.mFilter, min, mag, desc.mMipmaps);
	
	GLenum channels;
	GLenum type;
	TypeToGL(desc.mFormat, channels, type);

	GLenum internal = FormatToInternalGL(desc.mFormat);

	GLuint texture;
	glCreateTextures(GL_TEXTURE_2D, 1, &texture);

	int mipmapLevels = 1;
	if (desc.mMipmaps)
	{
		float log = glm::log2((float)glm::max(desc.mWidth, desc.mHeight));
		mipmapLevels = 1 + static_cast<int>(glm::floor(log));
	}
	
	glTextureStorage2D(texture, mipmapLevels, internal, desc.mWidth, desc.mHeight);
	glTextureSubImage2D(texture, 0, 0, 0, desc.mWidth, desc.mHeight, channels, type, desc.mData);

	if (desc.mMipmaps)
	{
		glGenerateTextureMipmap(texture);
	}

	glTextureParameteri(texture, GL_TEXTURE_WRAP_S, WrapToGL(desc.mWrap));
	glTextureParameteri(texture, GL_TEXTURE_WRAP_T, WrapToGL(desc.mWrap));
	glTextureParameteri(texture, GL_TEXTURE_MIN_FILTER, min);
	glTextureParameteri(texture, GL_TEXTURE_MAG_FILTER, mag);
	if (desc.mWrap == TextureWrap::BORDER) 
	{
		glTextureParameterfv(texture, GL_TEXTURE_BORDER_COLOR, &desc.mBorderColor[0]);
	}

	TextureDesc d;
	d.mType = TextureType::Texture2D;
	d.desc2D = desc;
	textureDescriptions[{texture}] = std::move(d);
	
	return { texture };
}

void Renderer::DestroyTexture(TextureHandle handle)
{
	textureDescriptions.erase(handle);
	glDeleteTextures(1, &handle.idx);
}

TextureHandle Renderer::CreateTexture3D(const TextureDescription3D& desc)
{
	DEBUG_ASSERT(desc.mFormat != TextureFormat::INVALID, "Invalid texture format!");

	GLenum min;
	GLenum mag;
	FilterToGL(desc.mFilter, min, mag, desc.mMipmaps);

	GLenum channels;
	GLenum type;
	TypeToGL(desc.mFormat, channels, type);

	GLenum internal = FormatToInternalGL(desc.mFormat);

	TextureHandle texture;
	glCreateTextures(GL_TEXTURE_3D, 1, &texture.idx);

	int mipmapLevels = 1;
	if (desc.mMipmaps)
	{
		float log = glm::log2((float)glm::max(desc.mWidth, desc.mHeight));
		mipmapLevels = 1 + static_cast<int>(glm::floor(log));
	}

	glTextureStorage3D(texture.idx, mipmapLevels, internal, desc.mWidth, desc.mHeight, desc.mDepth);
	for (u32 i = 0; i < desc.mData.size(); ++i)
	{
		glTextureSubImage3D(texture.idx, 0, 0, 0, i, desc.mWidth, desc.mHeight, desc.mDepth, channels, type, desc.mData[i]);
	}
	

	if (desc.mMipmaps)
	{
		glGenerateTextureMipmap(texture.idx);
	}

	glTextureParameteri(texture.idx, GL_TEXTURE_WRAP_S, WrapToGL(desc.mWrap));
	glTextureParameteri(texture.idx, GL_TEXTURE_WRAP_T, WrapToGL(desc.mWrap));
	glTextureParameteri(texture.idx, GL_TEXTURE_WRAP_R, WrapToGL(desc.mWrap));
	glTextureParameteri(texture.idx, GL_TEXTURE_MIN_FILTER, min);
	glTextureParameteri(texture.idx, GL_TEXTURE_MAG_FILTER, mag);
	if (desc.mWrap == TextureWrap::BORDER)
	{
		glTextureParameterfv(texture.idx, GL_TEXTURE_BORDER_COLOR, &desc.mBorderColor[0]);
	}

	TextureDesc d;
	d.mType = TextureType::Texture3D;
	d.desc3D = desc;
	textureDescriptions[texture] = std::move(d);

	return texture;
}


FrameBuffer Renderer::CreateFramebuffer(const FrameBufferDescription& desc)
{
	auto attachmentToGL = [](FramebufferAttachment attachment)
	{
		switch (attachment)
		{
		case FramebufferAttachment::COLOR0: return GL_COLOR_ATTACHMENT0;
		case FramebufferAttachment::COLOR1: return GL_COLOR_ATTACHMENT1;
		case FramebufferAttachment::COLOR2: return GL_COLOR_ATTACHMENT2;
		case FramebufferAttachment::COLOR3: return GL_COLOR_ATTACHMENT3;
		case FramebufferAttachment::DEPTH: return GL_DEPTH_ATTACHMENT;
		}

		DEBUG_ASSERT(false, "invalid FramebufferAttachment");
		return GL_INVALID_ENUM;
	};

	FrameBuffer result{};
	glGenFramebuffers(1, &result.mHandle);
	glBindFramebuffer(GL_FRAMEBUFFER, result.mHandle);

	std::array<GLenum, static_cast<u64>(OutputSlot::Count)> buffers;
	for (u64 i = 0; i < desc.mTextures.size(); ++i)
	{
		buffers[i] = GL_NONE;

		const auto& texConfig = desc.mTextures[i].mDescription;

		if (texConfig.mFormat == TextureFormat::INVALID) continue;

		GLenum attachment = attachmentToGL(desc.mTextures[i].mAttachment);
		TextureHandle texture = CreateTexture2D(texConfig);

		glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, texture.idx, 0);

		result.mWidth  = texConfig.mWidth;
		result.mHeight = texConfig.mHeight;
		result.mTextures[i] = texture;
		if (desc.mTextures[i].mAttachment != FramebufferAttachment::DEPTH)
		{
			buffers[i] = attachment;
		}
	}

	glDrawBuffers(static_cast<GLsizei>(buffers.size()), &buffers[0]);

	DEBUG_ASSERT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "Framebuffer creation failed!");
	DEBUG_ASSERT(result.mWidth > 0 && result.mHeight > 0, "Invalid framebuffer size!");

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	return result;
}

FrameBuffer Renderer::CreateFramebuffer(const TextureDescription2D& desc, FramebufferAttachment attachment)
{
	FrameBufferDescription description;
	description.mTextures[0].mDescription = desc;
	description.mTextures[0].mAttachment = attachment;

	return CreateFramebuffer(description);
}

void Renderer::DestroyFramebuffer(FrameBuffer buffer)
{
	if (!buffer.mHandle) return;

	glDeleteFramebuffers(1, &buffer.mHandle);
	for (const auto& tex : buffer.mTextures)
	{
		if (tex.idx)
		{
			DestroyTexture(tex);
		}
	}
}

ShaderHandle Renderer::CreateShader(const char* vertexSrc, const char* fragSrc, const char* tessCtrlSrc, const char* tessEvalSrc)
{
	auto createShader = [](GLenum shaderType, const char* src)
	{
		GLuint result = glCreateShader(shaderType);

		glShaderSource(result, 1, &src, NULL);
		glCompileShader(result);
		
		GLint status;
		glGetShaderiv(result, GL_COMPILE_STATUS, &status);
		
		if (status == GL_FALSE) 
		{
			char buf[1024];
			glGetShaderInfoLog(result, sizeof(buf), NULL, buf);

			auto err = "shader compilation failed: " + std::string(buf);
			std::cout << err << std::endl;
			glDeleteShader(result);
			return GLuint(0);
		}
		
		return result;
	};

	GLuint vert = createShader(GL_VERTEX_SHADER, vertexSrc);
	GLuint frag = createShader(GL_FRAGMENT_SHADER, fragSrc);

	if (tessCtrlSrc || tessEvalSrc)
	{
		DEBUG_ASSERT(tessCtrlSrc && tessEvalSrc, "Must have both control and eval shaders!");
	}

	GLuint tessCtrl = tessCtrlSrc ? createShader(GL_TESS_CONTROL_SHADER, tessCtrlSrc) : 0;
	GLuint tessEval = tessEvalSrc ? createShader(GL_TESS_EVALUATION_SHADER, tessEvalSrc) : 0;

	//shader failed to create, return invalid shader
	DEBUG_ASSERT(vert && frag, "Shader creation failed!");
	if (!vert || !frag) return {};

	// we must either have both or none
	if (tessCtrlSrc || tessEvalSrc)
	{
		DEBUG_ASSERT(tessCtrl && tessEval, "Shader creation failed!");
		if (!tessCtrl || !tessEval) return {};
	}
	
	GLuint program = glCreateProgram();
	glAttachShader(program, vert);
	glAttachShader(program, frag);
	if (tessCtrl) glAttachShader(program, tessCtrl);
	if (tessEval) glAttachShader(program, tessEval);

	glBindAttribLocation(program, VERTEX_ATTR_POSITION, "a_position");
	glBindAttribLocation(program, VERTEX_ATTR_NORMAL, "a_normal");
	glBindAttribLocation(program, VERTEX_ATTR_TEX_COORD0, "a_texcoord0");
	glBindAttribLocation(program, VERTEX_ATTR_TEX_COORD1, "a_texcoord1");
	glBindAttribLocation(program, VERTEX_ATTR_COLOR, "a_color");
	glBindAttribLocation(program, VERTEX_ATTR_JOINTS, "a_joints");
	glBindAttribLocation(program, VERTEX_ATTR_WEIGHTS, "a_weights");
	glBindAttribLocation(program, VERTEX_ATTR_MODEL_TO_WORLD_COL0, "a_model_to_world");

	glBindFragDataLocation(program, 0, "color0");
	glBindFragDataLocation(program, 1, "color1");
	glBindFragDataLocation(program, 2, "color2");
	glBindFragDataLocation(program, 3, "color3");

	glLinkProgram(program);

	glDeleteShader(vert);
	glDeleteShader(frag);
	if (tessCtrl) glDeleteShader(tessCtrl);
	if (tessEval) glDeleteShader(tessEval);

	//program failed to link, return invalid shader
	GLint status;
	glGetProgramiv(program, GL_LINK_STATUS, &status);
	if (status == GL_FALSE) 
	{
		char buf[1024];
		glGetProgramInfoLog(program, sizeof(buf), NULL, buf);
		
		std::string err = "shader linking failed: " + std::string(buf);
		std::cout << err << std::endl;
		
		return { };
	}

	glUseProgram(program);

	Shader& result = shaders[{program}];
	result.mHandle.idx = program;
	result.mTesselation = (tessCtrlSrc && tessEvalSrc);

	GatherShaderTextures(program, result);
	GatherShaderUniformBlocks(program, result);
	GatherShaderStorageBlocks(program, result);
	
	glUseProgram(0);

	return result.mHandle;
}

ShaderHandle Renderer::CreateComputeShader(const char* src)
{
	auto createShader = [](GLenum shaderType, const char* src)
	{
		GLuint result = glCreateShader(shaderType);

		glShaderSource(result, 1, &src, NULL);
		glCompileShader(result);

		GLint status;
		glGetShaderiv(result, GL_COMPILE_STATUS, &status);

		if (status == GL_FALSE)
		{
			char buf[1024];
			glGetShaderInfoLog(result, sizeof(buf), NULL, buf);

			auto err = "shader compilation failed: " + std::string(buf);
			std::cout << err << std::endl;
			glDeleteShader(result);
			return GLuint(0);
		}

		return result;
	};

	auto shader = createShader(GL_COMPUTE_SHADER, src);

	DEBUG_ASSERT(shader, "Shader creation failed!");
	if (!shader) return {};

	GLuint program = glCreateProgram();
	glAttachShader(program, shader);

	glLinkProgram(program);

	glDeleteShader(shader);

	//program failed to link, return invalid shader
	GLint status;
	glGetProgramiv(program, GL_LINK_STATUS, &status);
	if (status == GL_FALSE)
	{
		char buf[1024];
		glGetProgramInfoLog(program, sizeof(buf), NULL, buf);

		std::string err = "shader linking failed: ";
		err.append(buf);

		std::cout << err << std::endl;

		return { };
	}

	glUseProgram(program);

	Shader& result = shaders[{program}];;
	result.mHandle.idx = program;


	GatherShaderTextures(program, result);
	GatherShaderImages(program, result);
	GatherShaderUniformBlocks(program, result);
	GatherShaderStorageBlocks(program, result);

	glUseProgram(0);

	return result.mHandle;
}

void Renderer::DestroyShader(ShaderHandle shader)
{
	shaders.erase(shader);
	glDeleteProgram(shader.idx);
}

MeshHandle Renderer::CreateMesh(const MeshDescription& desc)
{
	auto vertexFormatGL = [](VertexFormat format, GLenum& type, GLint& components)
	{
		switch( format ) 
		{
		case VertexFormat::U8x3:
   			type = GL_UNSIGNED_BYTE;
   			components = 3;
   			break;
   		case VertexFormat::U8x4:
   			type = GL_UNSIGNED_BYTE;
   			components = 4;
   			break;
   		case VertexFormat::U16x4:
   			type = GL_UNSIGNED_SHORT;
   			components = 4;
   			break;
   		case VertexFormat::HALFx2:
   			type = GL_HALF_FLOAT;
   			components = 2;
   			break;
   		case VertexFormat::HALFx3:
   			type = GL_HALF_FLOAT;
   			components = 3;
   			break;
   		case VertexFormat::HALFx4:
   			type = GL_HALF_FLOAT;
   			components = 4;
   			break;
		case VertexFormat::FLOAT:
			type = GL_FLOAT;
			components = 1;
			break;
   		case VertexFormat::FLOATx2:
   			type = GL_FLOAT;
   			components = 2;
   			break;
   		case VertexFormat::FLOATx3:
   			type = GL_FLOAT;
   			components = 3;
   			break;
   		case VertexFormat::FLOATx4:
   			type = GL_FLOAT;
   			components = 4;
   			break;
		}
	};

	auto vertexAttribConfig = [&](GLuint index, VertexFormat format, u32 stride = 0, u32 offset = 0)
	{
		GLenum type;
		GLint components;
		vertexFormatGL(format, type, components);

		glEnableVertexAttribArray(index);

		if (index == VERTEX_ATTR_JOINTS)
		{
			glVertexAttribIPointer(index, components, type, stride, reinterpret_cast<void*>((u64)offset));
		}
		else
		{
			GLboolean normalized = (index == VERTEX_ATTR_WEIGHTS);
			glVertexAttribPointer(index, components, type, normalized, stride, reinterpret_cast<void*>((u64)offset));
		}
	};


	DEBUG_ASSERT(desc.mVertexCount, "No vertices found in mesh!");
	
	switch (desc.mPrimitiveType)
	{
	case PrimitiveType::TRIANGLES:
		if (!desc.mIndices.idx) DEBUG_ASSERT(desc.mVertexCount % 3 == 0, "Invalid vertex count!");
		break;
	case PrimitiveType::TRIANGLE_STRIP:
		DEBUG_ASSERT(desc.mVertexCount >= 3, "Invalid vertex count!");
		break;
	case PrimitiveType::POINTS:
		break; //no constraints 
	case PrimitiveType::LINES:
		if (!desc.mIndices.idx) DEBUG_ASSERT(desc.mVertexCount % 2 == 0, "Invalid vertex count!");
		break;
	}

	DEBUG_ASSERT(desc.mInterlacedBuffer.idx || desc.handles.mPositions.idx, "Must have position buffer!");

	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	// vertex buffers
	// TODO (danielg): Use DSA when creating vertex arrays
	if (!desc.mInterlacedBuffer.idx)
	{
		if (desc.handles.mPositions.idx)
		{
			glBindBuffer(GL_ARRAY_BUFFER, desc.handles.mPositions.idx);
			vertexAttribConfig(VERTEX_ATTR_POSITION, desc.mPositionFormat);
		}

		if (desc.handles.mNormals.idx)
		{
			glBindBuffer(GL_ARRAY_BUFFER, desc.handles.mNormals.idx);
			vertexAttribConfig(VERTEX_ATTR_NORMAL, desc.mNormalsFormat);
		}

		if (desc.handles.mTexCoords0.idx)
		{
			glBindBuffer(GL_ARRAY_BUFFER, desc.handles.mTexCoords0.idx);
			vertexAttribConfig(VERTEX_ATTR_TEX_COORD0, desc.mTexCoord0Format);
		}

		if (desc.handles.mTexCoords1.idx)
		{
			glBindBuffer(GL_ARRAY_BUFFER, desc.handles.mTexCoords1.idx);
			vertexAttribConfig(VERTEX_ATTR_TEX_COORD1, desc.mTexCoord1Format);
		}

		if (desc.handles.mColors.idx)
		{
			glBindBuffer(GL_ARRAY_BUFFER, desc.handles.mColors.idx);
			vertexAttribConfig(VERTEX_ATTR_COLOR, desc.mColorsFormat);
		}

		if (desc.handles.mJoints.idx)
		{
			glBindBuffer(GL_ARRAY_BUFFER, desc.handles.mJoints.idx);
			vertexAttribConfig(VERTEX_ATTR_JOINTS, desc.mJointsFormat);
		}

		if (desc.handles.mWeights.idx)
		{
			glBindBuffer(GL_ARRAY_BUFFER, desc.handles.mWeights.idx);
			vertexAttribConfig(VERTEX_ATTR_WEIGHTS, desc.mWeightsFormat);
		}
	}
	else
	{
		// interlaced vertex buffer
		DEBUG_ASSERT(desc.mStride, "Interlaced buffer must have stride!");
		glBindBuffer(GL_ARRAY_BUFFER, desc.mInterlacedBuffer.idx);

		constexpr u32 NO_OFFSET = std::numeric_limits<u32>::max();

		if (desc.offsets.mPositionOffset != NO_OFFSET)
		{
			vertexAttribConfig(VERTEX_ATTR_POSITION, desc.mPositionFormat, desc.mStride, desc.offsets.mPositionOffset);
		}
		
		if (desc.offsets.mNormalsOffset != NO_OFFSET)
		{
			vertexAttribConfig(VERTEX_ATTR_NORMAL, desc.mNormalsFormat, desc.mStride, desc.offsets.mNormalsOffset);
		}

		if (desc.offsets.mTexCoord0Offset != NO_OFFSET)
		{
			vertexAttribConfig(VERTEX_ATTR_TEX_COORD0, desc.mTexCoord0Format, desc.mStride, desc.offsets.mTexCoord0Offset);
		}

		if (desc.offsets.mTexCoord1Offset != NO_OFFSET)
		{
			vertexAttribConfig(VERTEX_ATTR_TEX_COORD1, desc.mTexCoord1Format, desc.mStride, desc.offsets.mTexCoord1Offset);
		}

		if (desc.offsets.mColorsOffset != NO_OFFSET)
		{
			vertexAttribConfig(VERTEX_ATTR_COLOR, desc.mColorsFormat, desc.mStride, desc.offsets.mColorsOffset);
		}

		if (desc.offsets.mJointsOffset != NO_OFFSET)
		{
			vertexAttribConfig(VERTEX_ATTR_JOINTS, desc.mJointsFormat, desc.mStride, desc.offsets.mJointsOffset);
		}

		if (desc.offsets.mWeightsOffset != NO_OFFSET)
		{
			vertexAttribConfig(VERTEX_ATTR_WEIGHTS, desc.mWeightsFormat, desc.mStride, desc.offsets.mWeightsOffset);
		}
	}

	// index buffer 
	if (desc.mIndices.idx)
	{
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, desc.mIndices.idx);
	}

	glBindVertexArray(0);

	
	Mesh& mesh = meshes[{vao}];

	mesh.mVertexCount	= desc.mVertexCount;
	mesh.mPrimitiveType = desc.mPrimitiveType;
	mesh.mIndices		= desc.mIndices;
	mesh.mIndexCount	= desc.mIndexCount;
	mesh.mIndexStart	= desc.mIndexStart;
	mesh.mIndexFormat	= desc.mIndicesFormat;
	mesh.mID			= vao;

	if (desc.mInterlacedBuffer.idx)
	{
		mesh.mPositions = desc.mInterlacedBuffer;
	}
	else
	{
		mesh.mPositions		= desc.handles.mPositions;
		mesh.mNormals		= desc.handles.mNormals;
		mesh.mTexCoords0	= desc.handles.mTexCoords0;
		mesh.mTexCoords1	= desc.handles.mTexCoords1;
		mesh.mColors		= desc.handles.mColors;
		mesh.mJoints		= desc.handles.mJoints;
		mesh.mWeights		= desc.handles.mWeights;
	}

	return { mesh.mID };
}

void Renderer::DestroyMesh(const MeshHandle mesh)
{
	DeleteCommand command;
	command.mType = DeleteCommand::Type::Mesh;
	command.mMesh = mesh;

	deletions.push_back(command);
}

void Renderer::DrawMesh(MeshHandle mesh, const RenderState& state, std::function<void()> preAction)
{
	DEBUG_ASSERT(buildingFrame, "Cannot submit draw if a frame is not in flight");
	DEBUG_ASSERT(state.mRenderPass != std::numeric_limits<u8>::max(), "Invalid render pass");
	DEBUG_ASSERT(state.mShader.idx, "invalid shader!");

	DrawCall draw;
	draw.mMesh = mesh;
	draw.mState = state;
	draw.mInstanceCount = 0;
	draw.mInstanceData = { 0 };
	draw.mPreAction = preAction;

	drawCalls.push_back(draw);
}

void Renderer::DrawMeshInstanced(MeshHandle mesh, const RenderState& state, VertexBufferHandle data, u32 instanceCount, std::function<void()> preAction)
{
	DEBUG_ASSERT(buildingFrame, "Cannot submit draw if a frame is not in flight");
	DEBUG_ASSERT(state.mRenderPass != std::numeric_limits<u8>::max(), "Invalid render pass");
	DEBUG_ASSERT(state.mShader.idx, "invalid shader!");

	if (!instanceCount) return;

	DrawCall draw;
	draw.mMesh = mesh;
	draw.mState = state;
	draw.mInstanceCount = instanceCount;
	draw.mInstanceData = data; 
	draw.mPreAction = preAction;

	drawCalls.push_back(draw);
}


void Renderer::DispatchCompute(const RenderState& state, u16 groupsX, u16 groupsY, u16 groupsZ, std::function<void()> preAction)
{
	DEBUG_ASSERT(buildingFrame, "Cannot submit draw if a frame is not in flight");
	DEBUG_ASSERT(state.mRenderPass != std::numeric_limits<u8>::max(), "Invalid render pass");
	DEBUG_ASSERT(state.mShader.idx, "invalid shader!");

	DrawCall draw;
	draw.isCompute = true;
	draw.groupsX = glm::max((u16)1, groupsX);
	draw.groupsY = glm::max((u16)1, groupsY);
	draw.groupsZ = glm::max((u16)1, groupsZ);
					  
	draw.mState = state;
	draw.mInstanceCount = 0;
	draw.mInstanceData = { 0 };
	draw.mPreAction = preAction;

	drawCalls.push_back(draw);
}
