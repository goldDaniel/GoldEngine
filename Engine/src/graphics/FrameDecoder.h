#pragma once 


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
		static void Decode(graphics::Renderer& renderer, ServerResources& resources, BinaryReader& reader);
	};
}