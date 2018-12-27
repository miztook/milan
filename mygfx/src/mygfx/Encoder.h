#pragma once

#include "base.h"
#include "Frame.h"

namespace mygfx
{
	BX_ALIGN_DECL_CACHE_LINE(struct) EncoderImpl
	{
		EncoderImpl()
		{
			discard();
		}

		void begin(Frame* _frame, uint8_t _idx)
		{
			m_frame = _frame;

			m_cpuTimeBegin = bx::getHPCounter();

			m_uniformIdx = _idx;
			m_uniformBegin = 0;
			m_uniformEnd = 0;

			UniformBuffer* uniformBuffer = m_frame->m_uniformBuffer[m_uniformIdx];
			uniformBuffer->reset();

			m_numSubmitted = 0;
			m_numDropped = 0;
		}

		void end(bool _finalize)
		{
			if (_finalize)
			{
				UniformBuffer* uniformBuffer = m_frame->m_uniformBuffer[m_uniformIdx];
				uniformBuffer->finish();

				m_cpuTimeEnd = bx::getHPCounter();
			}

			if (BX_ENABLED(MYGFX_CONFIG_DEBUG_OCCLUSION))
			{
				m_occlusionQuerySet.clear();
			}

			if (BX_ENABLED(MYGFX_CONFIG_DEBUG_UNIFORM))
			{
				m_uniformSet.clear();
			}
		}

		void setMarker(const char* _name)
		{
			UniformBuffer* uniformBuffer = m_frame->m_uniformBuffer[m_uniformIdx];
			uniformBuffer->writeMarker(_name);
		}

		void setUniform(UniformType::Enum _type, UniformHandle _handle, const void* _value, uint16_t _num)
		{
			if (BX_ENABLED(MYGFX_CONFIG_DEBUG_UNIFORM))
			{
				BX_CHECK(m_uniformSet.end() == m_uniformSet.find(_handle.idx)
					, "Uniform %d (%s) was already set for this draw call."
					, _handle.idx
					, getName(_handle)
				);
				m_uniformSet.insert(_handle.idx);
			}

			UniformBuffer::update(&m_frame->m_uniformBuffer[m_uniformIdx]);
			UniformBuffer* uniformBuffer = m_frame->m_uniformBuffer[m_uniformIdx];
			uniformBuffer->writeUniform(_type, _handle.idx, _value, _num);
		}

		void setState(uint64_t _state, uint32_t _rgba)
		{
			uint8_t blend = ((_state&MYGFX_STATE_BLEND_MASK) >> MYGFX_STATE_BLEND_SHIFT) & 0xff;
			uint8_t alphaRef = ((_state&MYGFX_STATE_ALPHA_REF_MASK) >> MYGFX_STATE_ALPHA_REF_SHIFT) & 0xff;
			// transparency sort order table
			m_key.m_trans = "\x0\x2\x2\x3\x3\x2\x3\x2\x3\x2\x2\x2\x2\x2\x2\x2\x2\x2\x2"[((blend) & 0xf) + (!!blend)] + !!alphaRef;
			m_draw.m_stateFlags = _state;
			m_draw.m_rgba = _rgba;
		}

		void setCondition(OcclusionQueryHandle _handle, bool _visible)
		{
			m_draw.m_occlusionQuery = _handle;
			m_draw.m_submitFlags |= _visible ? MYGFX_SUBMIT_INTERNAL_OCCLUSION_VISIBLE : 0;
		}

		void setStencil(uint32_t _fstencil, uint32_t _bstencil)
		{
			m_draw.m_stencil = packStencil(_fstencil, _bstencil);
		}

		uint16_t setScissor(uint16_t _x, uint16_t _y, uint16_t _width, uint16_t _height)
		{
			uint16_t scissor = (uint16_t)m_frame->m_frameCache.m_rectCache.add(_x, _y, _width, _height);
			m_draw.m_scissor = scissor;
			return scissor;
		}

		void setScissor(uint16_t _cache)
		{
			m_draw.m_scissor = _cache;
		}

		uint32_t setTransform(const void* _mtx, uint16_t _num)
		{
			m_draw.m_startMatrix = m_frame->m_frameCache.m_matrixCache.add(_mtx, _num);
			m_draw.m_numMatrices = _num;

			return m_draw.m_startMatrix;
		}

		uint32_t allocTransform(Transform* _transform, uint16_t _num)
		{
			uint32_t first = m_frame->m_frameCache.m_matrixCache.reserve(&_num);
			_transform->data = m_frame->m_frameCache.m_matrixCache.toPtr(first);
			_transform->num = _num;

			return first;
		}

		void setTransform(uint32_t _cache, uint16_t _num)
		{
			BX_CHECK(_cache < MYGFX_CONFIG_MAX_MATRIX_CACHE, "Matrix cache out of bounds index %d (max: %d)"
				, _cache
				, MYGFX_CONFIG_MAX_MATRIX_CACHE
			);
			m_draw.m_startMatrix = _cache;
			m_draw.m_numMatrices = uint16_t(bx::min<uint32_t>(_cache + _num, MYGFX_CONFIG_MAX_MATRIX_CACHE - 1) - _cache);
		}

		void setIndexBuffer(IndexBufferHandle _handle, uint32_t _firstIndex, uint32_t _numIndices)
		{
			BX_CHECK(UINT8_MAX != m_draw.m_streamMask, "");
			m_draw.m_startIndex = _firstIndex;
			m_draw.m_numIndices = _numIndices;
			m_draw.m_indexBuffer = _handle;
		}

		void setIndexBuffer(const DynamicIndexBuffer& _dib, uint32_t _firstIndex, uint32_t _numIndices)
		{
			BX_CHECK(UINT8_MAX != m_draw.m_streamMask, "");
			const uint32_t indexSize = 0 == (_dib.m_flags & MYGFX_BUFFER_INDEX32) ? 2 : 4;
			m_draw.m_startIndex = _dib.m_startIndex + _firstIndex;
			m_draw.m_numIndices = bx::min(_numIndices, _dib.m_size / indexSize);
			m_draw.m_indexBuffer = _dib.m_handle;
		}

		void setIndexBuffer(const TransientIndexBuffer* _tib, uint32_t _firstIndex, uint32_t _numIndices)
		{
			BX_CHECK(UINT8_MAX != m_draw.m_streamMask, "");
			const uint32_t numIndices = bx::min(_numIndices, _tib->size / 2);
			m_draw.m_indexBuffer = _tib->handle;
			m_draw.m_startIndex = _tib->startIndex + _firstIndex;
			m_draw.m_numIndices = numIndices;
			m_discard = 0 == numIndices;
		}

		void setVertexBuffer(uint8_t _stream, VertexBufferHandle _handle, uint32_t _startVertex, uint32_t _numVertices)
		{
			BX_CHECK(UINT8_MAX != m_draw.m_streamMask, "");
			BX_CHECK(_stream < MYGFX_CONFIG_MAX_VERTEX_STREAMS, "Invalid stream %d (max %d).", _stream, MYGFX_CONFIG_MAX_VERTEX_STREAMS);
			if (m_draw.setStreamBit(_stream, _handle))
			{
				Stream& stream = m_draw.m_stream[_stream];
				stream.m_startVertex = _startVertex;
				stream.m_handle = _handle;
				stream.m_decl.idx = kInvalidHandle;
				m_numVertices[_stream] = _numVertices;
			}
		}

		void setVertexBuffer(uint8_t _stream, const DynamicVertexBuffer& _dvb, uint32_t _startVertex, uint32_t _numVertices)
		{
			BX_CHECK(UINT8_MAX != m_draw.m_streamMask, "");
			BX_CHECK(_stream < MYGFX_CONFIG_MAX_VERTEX_STREAMS, "Invalid stream %d (max %d).", _stream, MYGFX_CONFIG_MAX_VERTEX_STREAMS);
			if (m_draw.setStreamBit(_stream, _dvb.m_handle))
			{
				Stream& stream = m_draw.m_stream[_stream];
				stream.m_startVertex = _dvb.m_startVertex + _startVertex;
				stream.m_handle = _dvb.m_handle;
				stream.m_decl = _dvb.m_decl;
				m_numVertices[_stream] =
					bx::min(bx::uint32_imax(0, _dvb.m_numVertices - _startVertex), _numVertices)
					;
			}
		}

		void setVertexBuffer(uint8_t _stream, const TransientVertexBuffer* _tvb, uint32_t _startVertex, uint32_t _numVertices)
		{
			BX_CHECK(UINT8_MAX != m_draw.m_streamMask, "");
			BX_CHECK(_stream < MYGFX_CONFIG_MAX_VERTEX_STREAMS, "Invalid stream %d (max %d).", _stream, MYGFX_CONFIG_MAX_VERTEX_STREAMS);
			if (m_draw.setStreamBit(_stream, _tvb->handle))
			{
				Stream& stream = m_draw.m_stream[_stream];
				stream.m_startVertex = _tvb->startVertex + _startVertex;
				stream.m_handle = _tvb->handle;
				stream.m_decl = _tvb->decl;
				m_numVertices[_stream] =
					bx::min(bx::uint32_imax(0, _tvb->size / _tvb->stride - _startVertex), _numVertices)
					;
			}
		}

		void setVertexCount(uint32_t _numVertices)
		{
			BX_CHECK(0 == m_draw.m_streamMask, "Vertex buffer already set.");
			m_draw.m_streamMask = UINT8_MAX;
			Stream& stream = m_draw.m_stream[0];
			stream.m_startVertex = 0;
			stream.m_handle.idx = kInvalidHandle;
			stream.m_decl.idx = kInvalidHandle;
			m_numVertices[0] = _numVertices;
		}

		void setInstanceDataBuffer(VertexBufferHandle _handle, uint32_t _startVertex, uint32_t _num, uint16_t _stride)
		{
			m_draw.m_instanceDataOffset = _startVertex * _stride;
			m_draw.m_instanceDataStride = _stride;
			m_draw.m_numInstances = _num;
			m_draw.m_instanceDataBuffer = _handle;
		}

		void setInstanceCount(uint32_t _numInstances)
		{
			BX_CHECK(!isValid(m_draw.m_instanceDataBuffer), "Instance buffer already set.");
			m_draw.m_numInstances = _numInstances;
		}

		void setTexture(uint8_t _stage, UniformHandle _sampler, TextureHandle _handle, uint32_t _flags)
		{
			Binding& bind = m_bind.m_bind[_stage];
			bind.m_idx = _handle.idx;
			bind.m_type = uint8_t(Binding::Texture);
			bind.m_un.m_draw.m_textureFlags = (_flags&MYGFX_SAMPLER_INTERNAL_DEFAULT)
				? MYGFX_SAMPLER_INTERNAL_DEFAULT
				: _flags
				;

			if (isValid(_sampler))
			{
				uint32_t stage = _stage;
				setUniform(UniformType::Int1, _sampler, &stage, 1);
			}
		}

		void setBuffer(uint8_t _stage, IndexBufferHandle _handle, Access::Enum _access)
		{
			Binding& bind = m_bind.m_bind[_stage];
			bind.m_idx = _handle.idx;
			bind.m_type = uint8_t(Binding::IndexBuffer);
			bind.m_un.m_compute.m_format = 0;
			bind.m_un.m_compute.m_access = uint8_t(_access);
			bind.m_un.m_compute.m_mip = 0;
		}

		void setBuffer(uint8_t _stage, VertexBufferHandle _handle, Access::Enum _access)
		{
			Binding& bind = m_bind.m_bind[_stage];
			bind.m_idx = _handle.idx;
			bind.m_type = uint8_t(Binding::VertexBuffer);
			bind.m_un.m_compute.m_format = 0;
			bind.m_un.m_compute.m_access = uint8_t(_access);
			bind.m_un.m_compute.m_mip = 0;
		}

		void setImage(uint8_t _stage, TextureHandle _handle, uint8_t _mip, Access::Enum _access, TextureFormat::Enum _format)
		{
			Binding& bind = m_bind.m_bind[_stage];
			bind.m_idx = _handle.idx;
			bind.m_type = uint8_t(Binding::Image);
			bind.m_un.m_compute.m_format = uint8_t(_format);
			bind.m_un.m_compute.m_access = uint8_t(_access);
			bind.m_un.m_compute.m_mip = _mip;
		}

		void discard()
		{
			if (BX_ENABLED(MYGFX_CONFIG_DEBUG_UNIFORM))
			{
				m_uniformSet.clear();
			}

			m_discard = false;
			m_draw.clear();
			m_compute.clear();
			m_bind.clear();
		}

		void submit(ViewId _id, ProgramHandle _program, OcclusionQueryHandle _occlusionQuery, uint32_t _depth, bool _preserveState);

		// 		void submit(ViewId _id, ProgramHandle _program, IndirectBufferHandle _indirectHandle, uint16_t _start, uint16_t _num, uint32_t _depth, bool _preserveState)
		// 		{
		// 			m_draw.m_startIndirect = _start;
		// 			m_draw.m_numIndirect = _num;
		// 			m_draw.m_indirectBuffer = _indirectHandle;
		// 			OcclusionQueryHandle handle = MYGFX_INVALID_HANDLE;
		// 			submit(_id, _program, handle, _depth, _preserveState);
		// 		}

		void dispatch(ViewId _id, ProgramHandle _handle, uint32_t _ngx, uint32_t _ngy, uint32_t _ngz, uint8_t _flags);

		// 		void dispatch(ViewId _id, ProgramHandle _handle, IndirectBufferHandle _indirectHandle, uint16_t _start, uint16_t _num, uint8_t _flags)
		// 		{
		// 			m_compute.m_indirectBuffer = _indirectHandle;
		// 			m_compute.m_startIndirect = _start;
		// 			m_compute.m_numIndirect = _num;
		// 			dispatch(_id, _handle, 0, 0, 0, _flags);
		// 		}

		void blit(ViewId _id, TextureHandle _dst, uint8_t _dstMip, uint16_t _dstX, uint16_t _dstY, uint16_t _dstZ, TextureHandle _src, uint8_t _srcMip, uint16_t _srcX, uint16_t _srcY, uint16_t _srcZ, uint16_t _width, uint16_t _height, uint16_t _depth);

		Frame* m_frame;

		SortKey m_key;

		RenderDraw    m_draw;
		RenderCompute m_compute;
		RenderBind    m_bind;

		uint32_t m_numSubmitted;
		uint32_t m_numDropped;

		uint32_t m_uniformBegin;
		uint32_t m_uniformEnd;
		uint32_t m_numVertices[MYGFX_CONFIG_MAX_VERTEX_STREAMS];
		uint8_t  m_uniformIdx;
		bool     m_discard;

		typedef stl::unordered_set<uint16_t> HandleSet;
		HandleSet m_uniformSet;
		HandleSet m_occlusionQuerySet;

		int64_t m_cpuTimeBegin;
		int64_t m_cpuTimeEnd;
	};

}