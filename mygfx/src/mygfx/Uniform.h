#pragma once

#include "Base.h"

namespace mygfx
{
#define MYGFX_UNIFORM_FRAGMENTBIT UINT8_C(0x10)
#define MYGFX_UNIFORM_SAMPLERBIT  UINT8_C(0x20)
#define MYGFX_UNIFORM_MASK (MYGFX_UNIFORM_FRAGMENTBIT|MYGFX_UNIFORM_SAMPLERBIT)

#define CONSTANT_OPCODE_TYPE_SHIFT 27
#define CONSTANT_OPCODE_TYPE_MASK  UINT32_C(0xf8000000)
#define CONSTANT_OPCODE_LOC_SHIFT  11
#define CONSTANT_OPCODE_LOC_MASK   UINT32_C(0x07fff800)
#define CONSTANT_OPCODE_NUM_SHIFT  1
#define CONSTANT_OPCODE_NUM_MASK   UINT32_C(0x000007fe)
#define CONSTANT_OPCODE_COPY_SHIFT 0
#define CONSTANT_OPCODE_COPY_MASK  UINT32_C(0x00000001)

	struct PredefinedUniform
	{
		enum Enum
		{
			ViewRect,
			ViewTexel,
			View,
			InvView,
			Proj,
			InvProj,
			ViewProj,
			InvViewProj,
			Model,
			ModelView,
			ModelViewProj,
			AlphaRef,
			Count
		};

		uint32_t m_loc;
		uint16_t m_count;
		uint8_t m_type;
	};

	const char* getUniformTypeName(UniformType::Enum _enum);
	UniformType::Enum nameToUniformTypeEnum(const char* _name);
	const char* getPredefinedUniformName(PredefinedUniform::Enum _enum);
	PredefinedUniform::Enum nameToPredefinedUniformEnum(const char* _name);

	class UniformBuffer
	{
	public:
		static UniformBuffer* create(uint32_t _size = 1 << 20)
		{
			const uint32_t structSize = sizeof(UniformBuffer) - sizeof(UniformBuffer::m_buffer);

			uint32_t size = BX_ALIGN_16(_size);
			void*    data = BX_ALLOC(g_allocator, size + structSize);
			return BX_PLACEMENT_NEW(data, UniformBuffer)(size);
		}

		static void destroy(UniformBuffer* _uniformBuffer)
		{
			_uniformBuffer->~UniformBuffer();
			BX_FREE(g_allocator, _uniformBuffer);
		}

		static void update(UniformBuffer** _uniformBuffer, uint32_t _treshold = 64 << 10, uint32_t _grow = 1 << 20)
		{
			UniformBuffer* uniformBuffer = *_uniformBuffer;
			if (_treshold >= uniformBuffer->m_size - uniformBuffer->m_pos)
			{
				const uint32_t structSize = sizeof(UniformBuffer) - sizeof(UniformBuffer::m_buffer);
				uint32_t size = BX_ALIGN_16(uniformBuffer->m_size + _grow);
				void*    data = BX_REALLOC(g_allocator, uniformBuffer, size + structSize);
				uniformBuffer = reinterpret_cast<UniformBuffer*>(data);
				uniformBuffer->m_size = size;

				*_uniformBuffer = uniformBuffer;
			}
		}

		static uint32_t encodeOpcode(UniformType::Enum _type, uint16_t _loc, uint16_t _num, uint16_t _copy)
		{
			const uint32_t type = _type << CONSTANT_OPCODE_TYPE_SHIFT;
			const uint32_t loc = _loc << CONSTANT_OPCODE_LOC_SHIFT;
			const uint32_t num = _num << CONSTANT_OPCODE_NUM_SHIFT;
			const uint32_t copy = _copy << CONSTANT_OPCODE_COPY_SHIFT;
			return type | loc | num | copy;
		}

		static void decodeOpcode(uint32_t _opcode, UniformType::Enum& _type, uint16_t& _loc, uint16_t& _num, uint16_t& _copy)
		{
			const uint32_t type = (_opcode&CONSTANT_OPCODE_TYPE_MASK) >> CONSTANT_OPCODE_TYPE_SHIFT;
			const uint32_t loc = (_opcode&CONSTANT_OPCODE_LOC_MASK) >> CONSTANT_OPCODE_LOC_SHIFT;
			const uint32_t num = (_opcode&CONSTANT_OPCODE_NUM_MASK) >> CONSTANT_OPCODE_NUM_SHIFT;
			const uint32_t copy = (_opcode&CONSTANT_OPCODE_COPY_MASK); // >> CONSTANT_OPCODE_COPY_SHIFT;

			_type = (UniformType::Enum)(type);
			_copy = (uint16_t)copy;
			_num = (uint16_t)num;
			_loc = (uint16_t)loc;
		}

		void write(const void* _data, uint32_t _size)
		{
			BX_CHECK(m_pos + _size < m_size, "Write would go out of bounds. pos %d + size %d > max size: %d).", m_pos, _size, m_size);

			if (m_pos + _size < m_size)
			{
				bx::memCopy(&m_buffer[m_pos], _data, _size);
				m_pos += _size;
			}
		}

		void write(uint32_t _value)
		{
			write(&_value, sizeof(uint32_t));
		}

		const char* read(uint32_t _size)
		{
			BX_CHECK(m_pos < m_size, "Out of bounds %d (size: %d).", m_pos, m_size);
			const char* result = &m_buffer[m_pos];
			m_pos += _size;
			return result;
		}

		uint32_t read()
		{
			uint32_t result;
			bx::memCopy(&result, read(sizeof(uint32_t)), sizeof(uint32_t));
			return result;
		}

		bool isEmpty() const
		{
			return 0 == m_pos;
		}

		uint32_t getPos() const
		{
			return m_pos;
		}

		void reset(uint32_t _pos = 0)
		{
			m_pos = _pos;
		}

		void finish()
		{
			write(UniformType::End);
			m_pos = 0;
		}

		void writeUniform(UniformType::Enum _type, uint16_t _loc, const void* _value, uint16_t _num = 1);
		void writeUniformHandle(UniformType::Enum _type, uint16_t _loc, UniformHandle _handle, uint16_t _num = 1);
		void writeMarker(const char* _marker);

	private:
		UniformBuffer(uint32_t _size)
			: m_size(_size)
			, m_pos(0)
		{
			finish();
		}

		~UniformBuffer()
		{
		}

		uint32_t m_size;
		uint32_t m_pos;
		char m_buffer[256 << 20];
	};

	struct UniformRegInfo
	{
		UniformHandle m_handle;
	};

	class UniformRegistry
	{
	public:
		UniformRegistry()
		{
		}

		~UniformRegistry()
		{
		}

		const UniformRegInfo* find(const char* _name) const
		{
			uint16_t handle = m_uniforms.find(bx::hash<bx::HashMurmur2A>(_name));
			if (kInvalidHandle != handle)
			{
				return &m_info[handle];
			}

			return NULL;
		}

		const UniformRegInfo& add(UniformHandle _handle, const char* _name)
		{
			BX_CHECK(isValid(_handle), "Uniform handle is invalid (name: %s)!", _name);
			const uint32_t key = bx::hash<bx::HashMurmur2A>(_name);
			m_uniforms.removeByKey(key);
			m_uniforms.insert(key, _handle.idx);

			UniformRegInfo& info = m_info[_handle.idx];
			info.m_handle = _handle;

			return info;
		}

		void remove(UniformHandle _handle)
		{
			m_uniforms.removeByHandle(_handle.idx);
		}

	private:
		typedef bx::HandleHashMapT<MYGFX_CONFIG_MAX_UNIFORMS * 2> UniformHashMap;
		UniformHashMap m_uniforms;
		UniformRegInfo m_info[MYGFX_CONFIG_MAX_UNIFORMS];
	};
}