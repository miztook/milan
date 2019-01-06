#pragma once

#include "Base.h"
#include "Structs.h"
#include "Structs2.h"
#include "CommandBuffer.h"
#include "Uniform.h"

namespace mygfx
{
	BX_ALIGN_DECL_CACHE_LINE(struct) View
	{
		void reset()
		{
			setRect(0, 0, 1, 1);
			setScissor(0, 0, 0, 0);
			setClear(MYGFX_CLEAR_NONE, 0, 0.0f, 0);
			setMode(ViewMode::Default);
			setFrameBuffer(MYGFX_INVALID_HANDLE);
			setTransform(NULL, NULL, MYGFX_VIEW_NONE, NULL);
		}

		void setRect(uint16_t _x, uint16_t _y, uint16_t _width, uint16_t _height)
		{
			m_rect.m_x = uint16_t(bx::max<int16_t>(int16_t(_x), 0));
			m_rect.m_y = uint16_t(bx::max<int16_t>(int16_t(_y), 0));
			m_rect.m_width = bx::max<uint16_t>(_width, 1);
			m_rect.m_height = bx::max<uint16_t>(_height, 1);
		}

		void setScissor(uint16_t _x, uint16_t _y, uint16_t _width, uint16_t _height)
		{
			m_scissor.m_x = _x;
			m_scissor.m_y = _y;
			m_scissor.m_width = _width;
			m_scissor.m_height = _height;
		}

		void setClear(uint16_t _flags, uint32_t _rgba, float _depth, uint8_t _stencil)
		{
			m_clear.set(_flags, _rgba, _depth, _stencil);
		}

		void setClear(uint16_t _flags, float _depth, uint8_t _stencil, uint8_t _0, uint8_t _1, uint8_t _2, uint8_t _3, uint8_t _4, uint8_t _5, uint8_t _6, uint8_t _7)
		{
			m_clear.set(_flags, _depth, _stencil, _0, _1, _2, _3, _4, _5, _6, _7);
		}

		void setMode(ViewMode::Enum _mode)
		{
			m_mode = uint8_t(_mode);
		}

		void setFrameBuffer(FrameBufferHandle _handle)
		{
			m_fbh = _handle;
		}

		void setTransform(const void* _view, const void* _proj, uint8_t _flags, const void* _proj1)
		{
			m_flags = _flags;

			if (NULL != _view)
			{
				bx::memCopy(m_view.un.val, _view, sizeof(Matrix4));
			}
			else
			{
				m_view.setIdentity();
			}

			if (NULL != _proj)
			{
				bx::memCopy(m_proj[0].un.val, _proj, sizeof(Matrix4));
			}
			else
			{
				m_proj[0].setIdentity();
			}

			if (NULL != _proj1)
			{
				bx::memCopy(m_proj[1].un.val, _proj1, sizeof(Matrix4));
			}
			else
			{
				bx::memCopy(m_proj[1].un.val, m_proj[0].un.val, sizeof(Matrix4));
			}
		}

		Clear   m_clear;
		Rect    m_rect;
		Rect    m_scissor;
		Matrix4 m_view;
		Matrix4 m_proj[2];
		FrameBufferHandle m_fbh;
		uint8_t m_mode;
		uint8_t m_flags;
	};

	struct MatrixCache
	{
		MatrixCache()
			: m_num(1)
		{
			m_cache[0].setIdentity();
		}

		void reset()
		{
			m_num = 1;
		}

		uint32_t reserve(uint16_t* _num)
		{
			uint32_t num = *_num;
			uint32_t first = bx::atomicFetchAndAddsat<uint32_t>(&m_num, num, MYGFX_CONFIG_MAX_MATRIX_CACHE - 1);
			BX_WARN(first + num < MYGFX_CONFIG_MAX_MATRIX_CACHE, "Matrix cache overflow. %d (max: %d)", first + num, MYGFX_CONFIG_MAX_MATRIX_CACHE);
			num = bx::min(num, MYGFX_CONFIG_MAX_MATRIX_CACHE - 1 - first);
			*_num = (uint16_t)num;
			return first;
		}

		uint32_t add(const void* _mtx, uint16_t _num)
		{
			if (NULL != _mtx)
			{
				uint32_t first = reserve(&_num);
				bx::memCopy(&m_cache[first], _mtx, sizeof(Matrix4)*_num);
				return first;
			}

			return 0;
		}

		float* toPtr(uint32_t _cacheIdx)
		{
			BX_CHECK(_cacheIdx < MYGFX_CONFIG_MAX_MATRIX_CACHE, "Matrix cache out of bounds index %d (max: %d)"
				, _cacheIdx
				, MYGFX_CONFIG_MAX_MATRIX_CACHE
			);
			return m_cache[_cacheIdx].un.val;
		}

		uint32_t fromPtr(const void* _ptr) const
		{
			return uint32_t((const Matrix4*)_ptr - m_cache);
		}

		Matrix4 m_cache[MYGFX_CONFIG_MAX_MATRIX_CACHE];
		uint32_t m_num;
	};

	struct RectCache
	{
		RectCache()
			: m_num(0)
		{
		}

		void reset()
		{
			m_num = 0;
		}

		uint32_t add(uint16_t _x, uint16_t _y, uint16_t _width, uint16_t _height)
		{
			const uint32_t first = bx::atomicFetchAndAddsat<uint32_t>(&m_num, 1, MYGFX_CONFIG_MAX_RECT_CACHE - 1);
			BX_CHECK(first + 1 < MYGFX_CONFIG_MAX_RECT_CACHE, "Rect cache overflow. %d (max: %d)", first, MYGFX_CONFIG_MAX_RECT_CACHE);

			Rect& rect = m_cache[first];

			rect.m_x = _x;
			rect.m_y = _y;
			rect.m_width = _width;
			rect.m_height = _height;

			return first;
		}

		Rect m_cache[MYGFX_CONFIG_MAX_RECT_CACHE];
		uint32_t m_num;
	};

	struct FrameCache
	{
		void reset()
		{
			m_matrixCache.reset();
			m_rectCache.reset();
		}

		bool isZeroArea(const Rect& _rect, uint16_t _scissor) const
		{
			if (UINT16_MAX != _scissor)
			{
				Rect scissorRect;
				scissorRect.setIntersect(_rect, m_rectCache.m_cache[_scissor]);
				return scissorRect.isZeroArea();
			}

			return false;
		}

		MatrixCache m_matrixCache;
		RectCache m_rectCache;
	};

	BX_ALIGN_DECL_CACHE_LINE(struct) Frame
	{
		Frame()
			: m_waitSubmit(0)
			, m_waitRender(0)
			, m_capture(false)
		{
			SortKey term;
			term.reset();
			term.m_program = kInvalidHandle;
			m_sortKeys[MYGFX_CONFIG_MAX_DRAW_CALLS] = term.encodeDraw(SortKey::SortProgram);
			m_sortValues[MYGFX_CONFIG_MAX_DRAW_CALLS] = MYGFX_CONFIG_MAX_DRAW_CALLS;
			bx::memSet(m_occlusion, 0xff, sizeof(m_occlusion));

			m_perfStats.viewStats = m_viewStats;
		}

		~Frame()
		{
		}

		void create()
		{
			{
				const uint32_t num = g_caps.limits.maxEncoders;

				m_uniformBuffer = (UniformBuffer**)BX_ALLOC(g_allocator, sizeof(UniformBuffer*)*num);

				for (uint32_t ii = 0; ii < num; ++ii)
				{
					m_uniformBuffer[ii] = UniformBuffer::create();
				}
			}

			reset();
			start();
			m_textVideoMem = BX_NEW(g_allocator, TextVideoMem);
		}

		void destroy()
		{
			for (uint32_t ii = 0, num = g_caps.limits.maxEncoders; ii < num; ++ii)
			{
				UniformBuffer::destroy(m_uniformBuffer[ii]);
			}

			BX_FREE(g_allocator, m_uniformBuffer);
			BX_DELETE(g_allocator, m_textVideoMem);
		}

		void reset()
		{
			start();
			finish();
			resetFreeHandles();
		}

		void start()
		{
			m_perfStats.transientVbUsed = m_vboffset;
			m_perfStats.transientIbUsed = m_iboffset;

			m_frameCache.reset();
			m_numRenderItems = 0;
			m_numBlitItems = 0;
			m_iboffset = 0;
			m_vboffset = 0;
			m_cmdPre.start();
			m_cmdPost.start();
			m_capture = false;
		}

		void finish()
		{
			m_cmdPre.finish();
			m_cmdPost.finish();

			//			if (0 < m_numDropped)
			//			{
			//				BX_TRACE("Too many draw calls: %d, dropped %d (max: %d)"
			//					, m_numRenderItems+m_numDropped
			//					, m_numDropped
			//					, BGFX_CONFIG_MAX_DRAW_CALLS
			//					);
			//			}
		}

		void sort();

		uint32_t getAvailTransientIndexBuffer(uint32_t _num)
		{
			uint32_t offset = bx::strideAlign(m_iboffset, sizeof(uint16_t));
			uint32_t iboffset = offset + _num * sizeof(uint16_t);
			iboffset = bx::min<uint32_t>(iboffset, g_caps.limits.transientIbSize);
			uint32_t num = (iboffset - offset) / sizeof(uint16_t);
			return num;
		}

		uint32_t allocTransientIndexBuffer(uint32_t& _num)
		{
			uint32_t offset = bx::strideAlign(m_iboffset, sizeof(uint16_t));
			uint32_t num = getAvailTransientIndexBuffer(_num);
			m_iboffset = offset + num * sizeof(uint16_t);
			_num = num;

			return offset;
		}

		uint32_t getAvailTransientVertexBuffer(uint32_t _num, uint16_t _stride)
		{
			uint32_t offset = bx::strideAlign(m_vboffset, _stride);
			uint32_t vboffset = offset + _num * _stride;
			vboffset = bx::min<uint32_t>(vboffset, g_caps.limits.transientVbSize);
			uint32_t num = (vboffset - offset) / _stride;
			return num;
		}

		uint32_t allocTransientVertexBuffer(uint32_t& _num, uint16_t _stride)
		{
			uint32_t offset = bx::strideAlign(m_vboffset, _stride);
			uint32_t num = getAvailTransientVertexBuffer(_num, _stride);
			m_vboffset = offset + num * _stride;
			_num = num;

			return offset;
		}

		bool free(IndexBufferHandle _handle)
		{
			return m_freeIndexBuffer.queue(_handle);
		}

		bool free(VertexDeclHandle _handle)
		{
			return m_freeVertexDecl.queue(_handle);
		}

		bool free(VertexBufferHandle _handle)
		{
			return m_freeVertexBuffer.queue(_handle);
		}

		bool free(ShaderHandle _handle)
		{
			return m_freeShader.queue(_handle);
		}

		bool free(ProgramHandle _handle)
		{
			return m_freeProgram.queue(_handle);
		}

		bool free(TextureHandle _handle)
		{
			return m_freeTexture.queue(_handle);
		}

		bool free(FrameBufferHandle _handle)
		{
			return m_freeFrameBuffer.queue(_handle);
		}

		bool free(UniformHandle _handle)
		{
			return m_freeUniform.queue(_handle);
		}

		void resetFreeHandles()
		{
			m_freeIndexBuffer.reset();
			m_freeVertexDecl.reset();
			m_freeVertexBuffer.reset();
			m_freeShader.reset();
			m_freeProgram.reset();
			m_freeTexture.reset();
			m_freeFrameBuffer.reset();
			m_freeUniform.reset();
		}

		template<typename Ty, uint32_t Max>
		struct FreeHandle
		{
			FreeHandle()
				: m_num(0)
			{
			}

			bool isQueued(Ty _handle)
			{
				for (uint32_t ii = 0, num = m_num; ii < num; ++ii)
				{
					if (m_queue[ii].idx == _handle.idx)
					{
						return true;
					}
				}

				return false;
			}

			bool queue(Ty _handle)
			{
				if (BX_ENABLED(MYGFX_CONFIG_DEBUG))
				{
					if (isQueued(_handle))
					{
						return false;
					}
				}

				m_queue[m_num] = _handle;
				++m_num;

				return true;
			}

			void reset()
			{
				m_num = 0;
			}

			Ty get(uint16_t _idx) const
			{
				return m_queue[_idx];
			}

			uint16_t getNumQueued() const
			{
				return m_num;
			}

			Ty m_queue[Max];
			uint16_t m_num;
		};


		ViewId m_viewRemap[MYGFX_CONFIG_MAX_VIEWS];
		float m_colorPalette[MYGFX_CONFIG_MAX_COLOR_PALETTE][4];

		View m_view[MYGFX_CONFIG_MAX_VIEWS];

		int32_t m_occlusion[MYGFX_CONFIG_MAX_OCCLUSION_QUERIES];

		uint64_t m_sortKeys[MYGFX_CONFIG_MAX_DRAW_CALLS + 1];
		RenderItemCount m_sortValues[MYGFX_CONFIG_MAX_DRAW_CALLS + 1];
		RenderItem m_renderItem[MYGFX_CONFIG_MAX_DRAW_CALLS + 1];
		RenderBind m_renderItemBind[MYGFX_CONFIG_MAX_DRAW_CALLS + 1];

		uint32_t m_blitKeys[MYGFX_CONFIG_MAX_BLIT_ITEMS + 1];
		BlitItem m_blitItem[MYGFX_CONFIG_MAX_BLIT_ITEMS + 1];

		FrameCache m_frameCache;
		UniformBuffer** m_uniformBuffer;

		uint32_t m_numRenderItems;
		uint16_t m_numBlitItems;

		uint32_t m_iboffset;
		uint32_t m_vboffset;
		TransientIndexBuffer* m_transientIb;
		TransientVertexBuffer* m_transientVb;

		Resolution m_resolution;
		uint32_t m_debug;

		CommandBuffer m_cmdPre;
		CommandBuffer m_cmdPost;

		FreeHandle<IndexBufferHandle, MYGFX_CONFIG_MAX_INDEX_BUFFERS>  m_freeIndexBuffer;
		FreeHandle<VertexDeclHandle, MYGFX_CONFIG_MAX_VERTEX_DECLS>   m_freeVertexDecl;
		FreeHandle<VertexBufferHandle, MYGFX_CONFIG_MAX_VERTEX_BUFFERS> m_freeVertexBuffer;
		FreeHandle<ShaderHandle, MYGFX_CONFIG_MAX_SHADERS>        m_freeShader;
		FreeHandle<ProgramHandle, MYGFX_CONFIG_MAX_PROGRAMS>       m_freeProgram;
		FreeHandle<TextureHandle, MYGFX_CONFIG_MAX_TEXTURES>       m_freeTexture;
		FreeHandle<FrameBufferHandle, MYGFX_CONFIG_MAX_FRAME_BUFFERS>  m_freeFrameBuffer;
		FreeHandle<UniformHandle, MYGFX_CONFIG_MAX_UNIFORMS>       m_freeUniform;

		TextVideoMem* m_textVideoMem;

		Stats     m_perfStats;
		ViewStats m_viewStats[MYGFX_CONFIG_MAX_VIEWS];

		int64_t m_waitSubmit;
		int64_t m_waitRender;

		bool m_capture;
	};
};