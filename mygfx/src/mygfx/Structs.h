#pragma once

#include "Base.h"

namespace mygfx
{
	struct Handle
	{
		enum Enum
		{
			Shader,
			Texture,

			Count
		};

		uint16_t type;
		uint16_t idx;
	};

	inline Handle convert(ShaderHandle _handle)
	{
		Handle handle = { Handle::Shader, _handle.idx };
		return handle;
	}

	inline Handle convert(TextureHandle _handle)
	{
		Handle handle = { Handle::Texture, _handle.idx };
		return handle;
	}

	struct Clear
	{
		void set(uint16_t _flags, uint32_t _rgba, float _depth, uint8_t _stencil)
		{
			m_flags = _flags;
			m_index[0] = uint8_t(_rgba >> 24);
			m_index[1] = uint8_t(_rgba >> 16);
			m_index[2] = uint8_t(_rgba >> 8);
			m_index[3] = uint8_t(_rgba >> 0);
			m_depth = _depth;
			m_stencil = _stencil;
		}

		void set(uint16_t _flags, float _depth, uint8_t _stencil, uint8_t _0, uint8_t _1, uint8_t _2, uint8_t _3, uint8_t _4, uint8_t _5, uint8_t _6, uint8_t _7)
		{
			m_flags = (_flags & ~MYGFX_CLEAR_COLOR)
				| (0xff != (_0&_1&_2&_3&_4&_5&_6&_7) ? MYGFX_CLEAR_COLOR | MYGFX_CLEAR_COLOR_USE_PALETTE : 0)
				;
			m_index[0] = _0;
			m_index[1] = _1;
			m_index[2] = _2;
			m_index[3] = _3;
			m_index[4] = _4;
			m_index[5] = _5;
			m_index[6] = _6;
			m_index[7] = _7;
			m_depth = _depth;
			m_stencil = _stencil;
		}

		uint8_t  m_index[8];
		float    m_depth;
		uint8_t  m_stencil;
		uint16_t m_flags;
	};

	struct Rect
	{
		Rect()
		{
		}

		Rect(uint16_t _x, uint16_t _y, uint16_t _width, uint16_t _height)
			: m_x(_x)
			, m_y(_y)
			, m_width(_width)
			, m_height(_height)
		{
		}

		void clear()
		{
			m_x =
				m_y =
				m_width =
				m_height = 0;
		}

		bool isZero() const
		{
			uint64_t ui64 = *((uint64_t*)this);
			return UINT64_C(0) == ui64;
		}

		bool isZeroArea() const
		{
			return 0 == m_width
				|| 0 == m_height
				;
		}

		void set(uint16_t _x, uint16_t _y, uint16_t _width, uint16_t _height)
		{
			m_x = _x;
			m_y = _y;
			m_width = _width;
			m_height = _height;
		}

		void setIntersect(const Rect& _a, const Rect& _b)
		{
			const uint16_t sx = bx::max<uint16_t>(_a.m_x, _b.m_x);
			const uint16_t sy = bx::max<uint16_t>(_a.m_y, _b.m_y);
			const uint16_t ex = bx::min<uint16_t>(_a.m_x + _a.m_width, _b.m_x + _b.m_width);
			const uint16_t ey = bx::min<uint16_t>(_a.m_y + _a.m_height, _b.m_y + _b.m_height);
			m_x = sx;
			m_y = sy;
			m_width = (uint16_t)bx::uint32_satsub(ex, sx);
			m_height = (uint16_t)bx::uint32_satsub(ey, sy);
		}

		void intersect(const Rect& _a)
		{
			setIntersect(*this, _a);
		}

		uint16_t m_x;
		uint16_t m_y;
		uint16_t m_width;
		uint16_t m_height;
	};

	struct TextureCreate
	{
		TextureFormat::Enum m_format;
		uint16_t m_width;
		uint16_t m_height;
		uint16_t m_depth;
		uint16_t m_numLayers;
		uint8_t m_numMips;
		bool m_cubeMap;
		const Memory* m_mem;
	};

	template <uint32_t maxKeys>
	struct UpdateBatchT
	{
		UpdateBatchT()
			: m_num(0)
		{
		}

		void add(uint32_t _key, uint32_t _value)
		{
			uint32_t num = m_num++;
			m_keys[num] = _key;
			m_values[num] = _value;
		}

		bool sort()
		{
			if (0 < m_num)
			{
				uint32_t* tempKeys = (uint32_t*)alloca(sizeof(m_keys));
				uint32_t* tempValues = (uint32_t*)alloca(sizeof(m_values));
				bx::radixSort(m_keys, tempKeys, m_values, tempValues, m_num);
				return true;
			}

			return false;
		}

		bool isFull() const
		{
			return m_num >= maxKeys;
		}

		void reset()
		{
			m_num = 0;
		}

		uint32_t m_num;
		uint32_t m_keys[maxKeys];
		uint32_t m_values[maxKeys];
	};

	struct SortKey
	{
		enum Enum
		{
			SortProgram,
			SortDepth,
			SortSequence,
		};

		uint64_t encodeDraw(Enum _type)
		{
			if (SortDepth == _type)
			{
				const uint64_t depth = (uint64_t(m_depth) << SORT_KEY_DRAW_1_DEPTH_SHIFT) & SORT_KEY_DRAW_1_DEPTH_MASK;
				const uint64_t program = (uint64_t(m_program) << SORT_KEY_DRAW_1_PROGRAM_SHIFT) & SORT_KEY_DRAW_1_PROGRAM_MASK;
				const uint64_t trans = (uint64_t(m_trans) << SORT_KEY_DRAW_1_TRANS_SHIFT) & SORT_KEY_DRAW_1_TRANS_MASK;
				const uint64_t view = (uint64_t(m_view) << SORT_KEY_VIEW_SHIFT) & SORT_KEY_VIEW_MASK;
				const uint64_t key = view | SORT_KEY_DRAW_BIT | SORT_KEY_DRAW_TYPE_DEPTH | depth | trans | program;

				return key;
			}
			else if (SortSequence == _type)
			{
				const uint64_t seq = (uint64_t(m_seq) << SORT_KEY_DRAW_2_SEQ_SHIFT) & SORT_KEY_DRAW_2_SEQ_MASK;
				const uint64_t program = (uint64_t(m_program) << SORT_KEY_DRAW_2_PROGRAM_SHIFT) & SORT_KEY_DRAW_2_PROGRAM_MASK;
				const uint64_t trans = (uint64_t(m_trans) << SORT_KEY_DRAW_2_TRANS_SHIFT) & SORT_KEY_DRAW_2_TRANS_MASK;
				const uint64_t view = (uint64_t(m_view) << SORT_KEY_VIEW_SHIFT) & SORT_KEY_VIEW_MASK;
				const uint64_t key = view | SORT_KEY_DRAW_BIT | SORT_KEY_DRAW_TYPE_SEQUENCE | seq | trans | program;

				BX_CHECK(seq == (uint64_t(m_seq) << SORT_KEY_DRAW_2_SEQ_SHIFT)
					, "SortKey error, sequence is truncated (m_seq: %d)."
					, m_seq
				);

				return key;
			}

			const uint64_t depth = (uint64_t(m_depth) << SORT_KEY_DRAW_0_DEPTH_SHIFT) & SORT_KEY_DRAW_0_DEPTH_MASK;
			const uint64_t program = (uint64_t(m_program) << SORT_KEY_DRAW_0_PROGRAM_SHIFT) & SORT_KEY_DRAW_0_PROGRAM_MASK;
			const uint64_t trans = (uint64_t(m_trans) << SORT_KEY_DRAW_0_TRANS_SHIFT) & SORT_KEY_DRAW_0_TRANS_MASK;
			const uint64_t view = (uint64_t(m_view) << SORT_KEY_VIEW_SHIFT) & SORT_KEY_VIEW_MASK;
			const uint64_t key = view | SORT_KEY_DRAW_BIT | SORT_KEY_DRAW_TYPE_PROGRAM | trans | program | depth;

			return key;
		}

		uint64_t encodeCompute()
		{
			const uint64_t program = (uint64_t(m_program) << SORT_KEY_COMPUTE_PROGRAM_SHIFT) & SORT_KEY_COMPUTE_PROGRAM_MASK;
			const uint64_t seq = (uint64_t(m_seq) << SORT_KEY_COMPUTE_SEQ_SHIFT) & SORT_KEY_COMPUTE_SEQ_MASK;
			const uint64_t view = (uint64_t(m_view) << SORT_KEY_VIEW_SHIFT) & SORT_KEY_VIEW_MASK;
			const uint64_t key = program | seq | view;

			BX_CHECK(seq == (uint64_t(m_seq) << SORT_KEY_COMPUTE_SEQ_SHIFT)
				, "SortKey error, sequence is truncated (m_seq: %d)."
				, m_seq
			);

			return key;
		}

		/// Returns true if item is compute command.
		bool decode(uint64_t _key, ViewId _viewRemap[MYGFX_CONFIG_MAX_VIEWS])
		{
			m_view = _viewRemap[(_key & SORT_KEY_VIEW_MASK) >> SORT_KEY_VIEW_SHIFT];
			if (_key & SORT_KEY_DRAW_BIT)
			{
				uint64_t type = _key & SORT_KEY_DRAW_TYPE_MASK;
				if (type == SORT_KEY_DRAW_TYPE_DEPTH)
				{
					m_program = uint16_t((_key & SORT_KEY_DRAW_1_PROGRAM_MASK) >> SORT_KEY_DRAW_1_PROGRAM_SHIFT);
					return false;
				}
				else if (type == SORT_KEY_DRAW_TYPE_SEQUENCE)
				{
					m_program = uint16_t((_key & SORT_KEY_DRAW_2_PROGRAM_MASK) >> SORT_KEY_DRAW_2_PROGRAM_SHIFT);
					return false;
				}

				m_program = uint16_t((_key & SORT_KEY_DRAW_0_PROGRAM_MASK) >> SORT_KEY_DRAW_0_PROGRAM_SHIFT);
				return false; // draw
			}

			m_program = uint16_t((_key & SORT_KEY_COMPUTE_PROGRAM_MASK) >> SORT_KEY_COMPUTE_PROGRAM_SHIFT);
			return true; // compute
		}

		static ViewId decodeView(uint64_t _key)
		{
			return ViewId((_key & SORT_KEY_VIEW_MASK) >> SORT_KEY_VIEW_SHIFT);
		}

		static uint64_t remapView(uint64_t _key, ViewId _viewRemap[MYGFX_CONFIG_MAX_VIEWS])
		{
			const ViewId   oldView = decodeView(_key);
			const uint64_t view = uint64_t(_viewRemap[oldView]) << SORT_KEY_VIEW_SHIFT;
			const uint64_t key = (_key & ~SORT_KEY_VIEW_MASK) | view;
			return key;
		}

		void reset()
		{
			m_depth = 0;
			m_seq = 0;
			m_program = 0;
			m_view = 0;
			m_trans = 0;
		}

		uint32_t m_depth;
		uint32_t m_seq;
		uint16_t m_program;
		ViewId   m_view;
		uint8_t  m_trans;
	};

	struct BlitKey
	{
		uint32_t encode()
		{
			return 0
				| (uint32_t(m_view) << 24)
				| uint32_t(m_item)
				;
		}

		void decode(uint32_t _key)
		{
			m_item = uint16_t(_key & UINT16_MAX);
			m_view = ViewId(_key >> 24);
		}

		static uint32_t remapView(uint32_t _key, ViewId _viewRemap[MYGFX_CONFIG_MAX_VIEWS])
		{
			const ViewId   oldView = ViewId(_key >> 24);
			const uint32_t view = uint32_t(_viewRemap[oldView]) << 24;
			const uint32_t key = (_key & ~UINT32_C(0xff000000)) | view;
			return key;
		}

		uint16_t m_item;
		ViewId   m_view;
	};

	BX_ALIGN_DECL_16(struct) Srt
	{
		float rotate[4];
		float translate[3];
		float pad0;
		float scale[3];
		float pad1;
	};

	BX_ALIGN_DECL_16(struct) Matrix4
	{
		union
		{
			float val[16];
			bx::float4x4_t f4x4;
		} un;

		void setIdentity()
		{
			bx::memSet(un.val, 0, sizeof(un.val));
			un.val[0] = un.val[5] = un.val[10] = un.val[15] = 1.0f;
		}
	};

	struct Binding
	{
		enum Enum
		{
			Image,
			IndexBuffer,
			VertexBuffer,
			Texture,

			Count
		};

		uint16_t m_idx;
		uint8_t  m_type;

		union
		{
			struct
			{
				uint32_t m_textureFlags;
			} m_draw;

			struct
			{
				uint8_t  m_format;
				uint8_t  m_access;
				uint8_t  m_mip;
			} m_compute;

		} m_un;
	};

	struct Stream
	{
		void clear()
		{
			m_startVertex = 0;
			m_handle.idx = kInvalidHandle;
			m_decl.idx = kInvalidHandle;
		}

		uint32_t           m_startVertex;
		VertexBufferHandle m_handle;
		VertexDeclHandle   m_decl;
	};

	struct IndexBuffer
	{
		uint32_t m_size;
	};

	struct VertexBuffer
	{
		uint32_t m_size;
		uint16_t m_stride;
	};

	struct DynamicIndexBuffer
	{
		IndexBufferHandle m_handle;
		uint32_t m_offset;
		uint32_t m_size;
		uint32_t m_startIndex;
		uint16_t m_flags;
	};

	struct DynamicVertexBuffer
	{
		VertexBufferHandle m_handle;
		uint32_t m_offset;
		uint32_t m_size;
		uint32_t m_startVertex;
		uint32_t m_numVertices;
		uint16_t m_stride;
		VertexDeclHandle m_decl;
		uint16_t m_flags;
	};

	BX_ALIGN_DECL_CACHE_LINE(struct) RenderBind
	{
		void clear()
		{
			for (uint32_t ii = 0; ii < MYGFX_CONFIG_MAX_TEXTURE_SAMPLERS; ++ii)
			{
				Binding& bind = m_bind[ii];
				bind.m_idx = kInvalidHandle;
				bind.m_type = 0;
				bind.m_un.m_draw.m_textureFlags = 0;
			}
		};

		Binding m_bind[MYGFX_CONFIG_MAX_TEXTURE_SAMPLERS];
	};

	BX_ALIGN_DECL_CACHE_LINE(struct) RenderDraw
	{
		void clear()
		{
			m_uniformBegin = 0;
			m_uniformEnd = 0;
			m_stateFlags = MYGFX_STATE_DEFAULT;
			m_stencil = packStencil(MYGFX_STENCIL_DEFAULT, MYGFX_STENCIL_DEFAULT);
			m_rgba = 0;
			m_startMatrix = 0;
			m_startIndex = 0;
			m_numIndices = UINT32_MAX;
			m_numVertices = UINT32_MAX;
			m_instanceDataOffset = 0;
			m_instanceDataStride = 0;
			m_numInstances = 1;
			m_startIndirect = 0;
			m_numIndirect = UINT16_MAX;
			m_numMatrices = 1;
			m_submitFlags = MYGFX_SUBMIT_EYE_FIRST;
			m_scissor = UINT16_MAX;
			m_streamMask = 0;
			m_stream[0].clear();
			m_indexBuffer.idx = kInvalidHandle;
			m_instanceDataBuffer.idx = kInvalidHandle;
			//			m_indirectBuffer.idx = kInvalidHandle;
			m_occlusionQuery.idx = kInvalidHandle;
			m_uniformIdx = UINT8_MAX;
		}

		bool setStreamBit(uint8_t _stream, VertexBufferHandle _handle)
		{
			const uint8_t bit = 1 << _stream;
			const uint8_t mask = m_streamMask & ~bit;
			const uint8_t tmp = isValid(_handle) ? bit : 0;
			m_streamMask = mask | tmp;
			return 0 != tmp;
		}

		Stream   m_stream[MYGFX_CONFIG_MAX_VERTEX_STREAMS];
		uint64_t m_stateFlags;
		uint64_t m_stencil;
		uint32_t m_rgba;
		uint32_t m_uniformBegin;
		uint32_t m_uniformEnd;
		uint32_t m_startMatrix;
		uint32_t m_startIndex;
		uint32_t m_numIndices;
		uint32_t m_numVertices;
		uint32_t m_instanceDataOffset;
		uint32_t m_numInstances;
		uint16_t m_instanceDataStride;
		uint16_t m_startIndirect;
		uint16_t m_numIndirect;
		uint16_t m_numMatrices;
		uint16_t m_scissor;
		uint8_t  m_submitFlags;
		uint8_t  m_streamMask;
		uint8_t  m_uniformIdx;

		IndexBufferHandle    m_indexBuffer;
		VertexBufferHandle   m_instanceDataBuffer;
		//		IndirectBufferHandle m_indirectBuffer;
		OcclusionQueryHandle m_occlusionQuery;
	};

	BX_ALIGN_DECL_CACHE_LINE(struct) RenderCompute
	{
		void clear()
		{
			m_uniformBegin = 0;
			m_uniformEnd = 0;
			m_startMatrix = 0;
			m_numX = 0;
			m_numY = 0;
			m_numZ = 0;
			m_numMatrices = 0;
			m_submitFlags = MYGFX_SUBMIT_EYE_FIRST;
			m_uniformIdx = UINT8_MAX;

			//			m_indirectBuffer.idx = kInvalidHandle;
			m_startIndirect = 0;
			m_numIndirect = UINT16_MAX;
		}

		uint32_t m_uniformBegin;
		uint32_t m_uniformEnd;
		uint32_t m_startMatrix;
		//		IndirectBufferHandle m_indirectBuffer;

		uint32_t m_numX;
		uint32_t m_numY;
		uint32_t m_numZ;
		uint16_t m_startIndirect;
		uint16_t m_numIndirect;
		uint16_t m_numMatrices;
		uint8_t  m_submitFlags;
		uint8_t  m_uniformIdx;
	};

	union RenderItem
	{
		RenderDraw    draw;
		RenderCompute compute;
	};

	BX_ALIGN_DECL_CACHE_LINE(struct) BlitItem
	{
		uint16_t m_srcX;
		uint16_t m_srcY;
		uint16_t m_srcZ;
		uint16_t m_dstX;
		uint16_t m_dstY;
		uint16_t m_dstZ;
		uint16_t m_width;
		uint16_t m_height;
		uint16_t m_depth;
		uint8_t  m_srcMip;
		uint8_t  m_dstMip;
		TextureHandle m_src;
		TextureHandle m_dst;
	};
}