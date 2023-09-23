#pragma once 

#include "RenderCommands.h"

namespace graphics
{
	class Renderer;
}

namespace gold
{
	class LinearAllocator;
	class ServerResources;
	class BinaryReader;

	class FrameDecoder
	{
	
	public:
		static void Decode(graphics::Renderer& renderer, LinearAllocator& frameAllocator, ServerResources& resources, BinaryReader& reader, RenderCommand command);
	};
}