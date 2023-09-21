#pragma once

#include "core/Core.h"

namespace graphics
{
	class VertexLayout
	{
	public:
		enum ElementType
		{
			Position2 = 0,
			Position3,
			Texcoord2,
			Normal,
			Color3,
			Color4,
		};
		class Element
		{
		private:
			ElementType mType;
			uint32_t mOffset = 0;
		public:
			Element(ElementType type, uint32_t offset)
				: mType(type)
				, mOffset(offset) {}

			ElementType GetType() const { return mType; }

			uint32_t GetOffsetNext() const { return mOffset + Size(); }
			uint32_t GetOffset() const { return mOffset; }
			uint32_t Size() const { return SizeOfElement(mType); };

			static constexpr uint32_t SizeOfElement(ElementType type)
			{
				switch (type)
				{
				case ElementType::Position2: return static_cast<uint32_t>(sizeof(glm::vec2));
				case ElementType::Position3: return static_cast<uint32_t>(sizeof(glm::vec3));
				case ElementType::Texcoord2: return static_cast<uint32_t>(sizeof(glm::vec2));
				case ElementType::Normal:	 return static_cast<uint32_t>(sizeof(glm::vec3));
				case ElementType::Color3:	 return static_cast<uint32_t>(sizeof(glm::vec3));
				case ElementType::Color4:	 return static_cast<uint32_t>(sizeof(glm::vec4));
				}

				return 0;
			}
		};
	public:
		template<ElementType type>
		const Element& Resolve() const
		{
			for (const auto& e : mElements)
			{
				if (e.GetType() == type) return e;
			}

			DEBUG_ASSERT(false, "Could not resolve element type");
			return mElements.at(0);
		}

		const Element& Resolve(size_t index) const
		{
			return mElements.at(index);
		}

		template<ElementType type>
		bool HasElement() const
		{
			for (const auto& e : mElements)
			{
				if (e.GetType() == type) return true;
			}

			return false;
		}

		template<ElementType type>
		VertexLayout& Push()
		{
			mElements.emplace_back(type, Size());
			return *this;
		}

		uint32_t Size() const
		{
			return mElements.empty() ? 0 : static_cast<uint32_t>(mElements.back().GetOffsetNext());
		}

		uint32_t ElementCount() const { return static_cast<uint32_t>(mElements.size()); }

	private:
		std::vector<Element> mElements;
	};

	class Vertex
	{
		friend class VertexBuffer;
	public:

		template<VertexLayout::ElementType type>
		auto& Get()
		{
			const auto& element = mLayout.Resolve<type>();
			auto attribPtr = mData + element.GetOffset();

			if constexpr (type == VertexLayout::ElementType::Position2)
			{
				return *reinterpret_cast<glm::vec2*>(attribPtr);
			}
			if constexpr (type == VertexLayout::ElementType::Position3)
			{
				return *reinterpret_cast<glm::vec3*>(attribPtr);
			}
			if constexpr (type == VertexLayout::ElementType::Texcoord2)
			{
				return *reinterpret_cast<glm::vec2*>(attribPtr);
			}
			if constexpr (type == VertexLayout::ElementType::Normal)
			{
				return *reinterpret_cast<glm::vec3*>(attribPtr);
			}
			if constexpr (type == VertexLayout::ElementType::Color3)
			{
				return *reinterpret_cast<glm::vec3*>(attribPtr);
			}
			if constexpr (type == VertexLayout::ElementType::Color4)
			{
				return *reinterpret_cast<glm::vec4*>(attribPtr);
			}
;		}

	private:
		Vertex(uint8_t* data, const VertexLayout& layout)
			: mData(data)
			, mLayout(layout) {}

		template<typename T>
		void SetAttribByIndex(uint32_t index, T&& value)
		{
			const auto& element = mLayout.Resolve(index);
			uint8_t* attribPtr = mData + element.GetOffset();

			switch (element.GetType())
			{
			case VertexLayout::ElementType::Position2: SetAttrib<glm::vec2>(attribPtr, std::forward<T>(value)); break;
			case VertexLayout::ElementType::Position3: SetAttrib<glm::vec3>(attribPtr, std::forward<T>(value)); break;
			case VertexLayout::ElementType::Texcoord2: SetAttrib<glm::vec2>(attribPtr, std::forward<T>(value)); break;
			case VertexLayout::ElementType::Normal:	   SetAttrib<glm::vec3>(attribPtr, std::forward<T>(value)); break;
			case VertexLayout::ElementType::Color3:	   SetAttrib<glm::vec3>(attribPtr, std::forward<T>(value)); break;
			case VertexLayout::ElementType::Color4:    SetAttrib<glm::vec4>(attribPtr, std::forward<T>(value)); break;

			default: DEBUG_ASSERT(false, "Invalid Element Type");
			}
		}

		template<typename First, typename ...Rest>
		void SetAttribByIndex(uint32_t index, First&& first, Rest&&... remaining)
		{
			SetAttribByIndex(index, std::forward<First>(first));
			SetAttribByIndex(index + 1, std::forward<Rest>(remaining)...);
		}

		template<typename T, typename U>
		void SetAttrib(uint8_t* attribPtr, U&& value)
		{
			if constexpr (std::is_assignable<T, U>::value)
			{
				*reinterpret_cast<T*>(attribPtr) = value;
			}
			else
			{
				DEBUG_ASSERT(false, "Parameter - attribute mismatch!");
			}
		}

	private:
		uint8_t* mData = nullptr;
		const VertexLayout& mLayout;
	};

	class VertexBuffer
	{
	public:
		VertexBuffer(VertexLayout&& layout)
			: mLayout(std::move(layout)) { }

		const VertexLayout& GetLayout() const { return mLayout; }
		
		uint32_t VertexCount() const
		{
			size_t bufferSize = mBuffer.size();
			size_t vertexSize = mLayout.Size();
			return static_cast<uint32_t>(bufferSize / vertexSize);
		}

		uint32_t SizeInBytes() const
		{
			return static_cast<uint32_t>(mBuffer.size());
		}

		template<typename... Args>
		void Emplace(Args&&... args)
		{
			DEBUG_ASSERT(sizeof...(args) == mLayout.ElementCount(), "parameter count mismatch");

			mBuffer.resize(mBuffer.size() + mLayout.Size());
			Back().SetAttribByIndex(0u, std::forward<Args>(args)...);
		}

		Vertex Back()
		{
			DEBUG_ASSERT(mBuffer.size() > 0, "Emtpy buffer!");

			uint8_t* data = mBuffer.data() + mBuffer.size() - mLayout.Size();
			return Vertex{ data, mLayout };
		}

		Vertex Front()
		{
			DEBUG_ASSERT(mBuffer.size() > 0, "Empty Buffer!");

			uint8_t* data = mBuffer.data();
			return Vertex{ data, mLayout };
		}

		Vertex operator[](size_t index)
		{
			DEBUG_ASSERT(mBuffer.size() > 0 && index < VertexCount() , "");

			uint8_t* data = mBuffer.data() + mLayout.Size() * index;
			return Vertex{ data, mLayout };
		}

		uint8_t const* Raw() const
		{
			return &mBuffer[0];
		}

	private:
		std::vector<uint8_t> mBuffer;
		VertexLayout mLayout;
	};
}

