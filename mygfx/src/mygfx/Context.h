#pragma once

#include "Base.h"
#include "NonLocalAllocator.h"
#include "Frame.h"
#include "Encoder.h"

namespace mygfx
{
	struct RendererContextI;

	struct Context
	{
		Context();

		~Context()
		{
		}

		

		// game thread
		bool init(const Init& _init);
		void shutdown();

		CommandBuffer& getCommandBuffer(CommandBuffer::Enum _cmd);

		void reset(uint32_t _width, uint32_t _height, uint32_t _flags, TextureFormat::Enum _format);

		void setDebug(uint32_t _debug)
		{
			m_debug = _debug;
		}

		void dbgTextClear(uint8_t _attr, bool _small);

		void dbgTextPrintfVargs(uint16_t _x, uint16_t _y, uint8_t _attr, const char* _format, va_list _argList);

		void dbgTextImage(uint16_t _x, uint16_t _y, uint16_t _width, uint16_t _height, const void* _data, uint16_t _pitch);

		const Stats* getPerfStats();

		IndexBufferHandle createIndexBuffer(const Memory* _mem, uint16_t _flags);

		void destroyIndexBuffer(IndexBufferHandle _handle);

		VertexDeclHandle findVertexDecl(const VertexDecl& _decl);

		VertexBufferHandle createVertexBuffer(const Memory* _mem, const VertexDecl& _decl, uint16_t _flags);

		void destroyVertexBuffer(VertexBufferHandle _handle);

		void destroyVertexBufferInternal(VertexBufferHandle _handle);

		uint64_t allocDynamicIndexBuffer(uint32_t _size, uint16_t _flags);

		DynamicIndexBufferHandle createDynamicIndexBuffer(uint32_t _num, uint16_t _flags);

		DynamicIndexBufferHandle createDynamicIndexBuffer(const Memory* _mem, uint16_t _flags);

		void update(DynamicIndexBufferHandle _handle, uint32_t _startIndex, const Memory* _mem);

		void destroyDynamicIndexBuffer(DynamicIndexBufferHandle _handle);

		void destroyDynamicIndexBufferInternal(DynamicIndexBufferHandle _handle);

		uint64_t allocDynamicVertexBuffer(uint32_t _size, uint16_t _flags);

		DynamicVertexBufferHandle createDynamicVertexBuffer(uint32_t _num, const VertexDecl& _decl, uint16_t _flags);

		DynamicVertexBufferHandle createDynamicVertexBuffer(const Memory* _mem, const VertexDecl& _decl, uint16_t _flags);

		void update(DynamicVertexBufferHandle _handle, uint32_t _startVertex, const Memory* _mem);

		void destroyDynamicVertexBuffer(DynamicVertexBufferHandle _handle);

		void destroyDynamicVertexBufferInternal(DynamicVertexBufferHandle _handle);

		uint32_t getAvailTransientIndexBuffer(uint32_t _num);

		uint32_t getAvailTransientVertexBuffer(uint32_t _num, uint16_t _stride);

		TransientIndexBuffer* createTransientIndexBuffer(uint32_t _size);

		void destroyTransientIndexBuffer(TransientIndexBuffer* _tib);

		void allocTransientIndexBuffer(TransientIndexBuffer* _tib, uint32_t _num);

		TransientVertexBuffer* createTransientVertexBuffer(uint32_t _size, const VertexDecl* _decl = NULL);

		void destroyTransientVertexBuffer(TransientVertexBuffer* _tvb);

		void allocTransientVertexBuffer(TransientVertexBuffer* _tvb, uint32_t _num, const VertexDecl& _decl);

		ShaderHandle createShader(const Memory* _mem);

		uint16_t getShaderUniforms(ShaderHandle _handle, UniformHandle* _uniforms, uint16_t _max);

		void setName(Handle _handle, const bx::StringView& _name);

		void setName(ShaderHandle _handle, const bx::StringView& _name);

		void destroyShader(ShaderHandle _handle);

		void shaderTakeOwnership(ShaderHandle _handle);

		void shaderIncRef(ShaderHandle _handle);

		void shaderDecRef(ShaderHandle _handle);

		ProgramHandle createProgram(ShaderHandle _vsh, ShaderHandle _fsh, bool _destroyShaders);

		ProgramHandle createProgram(ShaderHandle _vsh, bool _destroyShader);

		void destroyProgram(ProgramHandle _handle);

		TextureHandle createTexture(const Memory* _mem, uint64_t _flags, uint8_t _skip, TextureInfo* _info, BackbufferRatio::Enum _ratio, bool _immutable);

		void setName(TextureHandle _handle, const bx::StringView& _name);

		void setDirectAccessPtr(TextureHandle _handle, void* _ptr);

		void* getDirectAccessPtr(TextureHandle _handle);

		void destroyTexture(TextureHandle _handle);

		uint32_t readTexture(TextureHandle _handle, void* _data, uint8_t _mip);

		void resizeTexture(TextureHandle _handle, uint16_t _width, uint16_t _height, uint8_t _numMips, uint16_t _numLayers);

		void textureTakeOwnership(TextureHandle _handle);

		void textureIncRef(TextureHandle _handle);

		void textureDecRef(TextureHandle _handle);

		void updateTexture(
			TextureHandle _handle
			, uint8_t _side
			, uint8_t _mip
			, uint16_t _x
			, uint16_t _y
			, uint16_t _z
			, uint16_t _width
			, uint16_t _height
			, uint16_t _depth
			, uint16_t _pitch
			, const Memory* _mem
		);

		bool checkFrameBuffer(uint8_t _num, const Attachment* _attachment) const;

		FrameBufferHandle createFrameBuffer(uint8_t _num, const Attachment* _attachment, bool _destroyTextures);

		FrameBufferHandle createFrameBuffer(void* _nwh, uint16_t _width, uint16_t _height, TextureFormat::Enum _format, TextureFormat::Enum _depthFormat);

		TextureHandle getTexture(FrameBufferHandle _handle, uint8_t _attachment);

		void destroyFrameBuffer(FrameBufferHandle _handle);

		UniformHandle createUniform(const char* _name, UniformType::Enum _type, uint16_t _num)
		{
			MYGFX_MUTEX_SCOPE(m_resourceApiLock);

			if (PredefinedUniform::Count != nameToPredefinedUniformEnum(_name))
			{
				BX_TRACE("%s is predefined uniform name.", _name);
				return MYGFX_INVALID_HANDLE;
			}

			_num = bx::max<uint16_t>(1, _num);

			uint16_t idx = m_uniformHashMap.find(bx::hash<bx::HashMurmur2A>(_name));
			if (kInvalidHandle != idx)
			{
				UniformHandle handle = { idx };
				UniformRef& uniform = m_uniformRef[handle.idx];
				BX_CHECK(uniform.m_type == _type
					, "Uniform type mismatch (type: %d, expected %d)."
					, _type
					, uniform.m_type
				);

				uint32_t oldsize = g_uniformTypeSize[uniform.m_type];
				uint32_t newsize = g_uniformTypeSize[_type];

				if (oldsize < newsize
					|| uniform.m_num < _num)
				{
					uniform.m_type = oldsize < newsize ? _type : uniform.m_type;
					uniform.m_num = bx::max<uint16_t>(uniform.m_num, _num);

					CommandBuffer& cmdbuf = getCommandBuffer(CommandBuffer::CreateUniform);
					cmdbuf.write(handle);
					cmdbuf.write(uniform.m_type);
					cmdbuf.write(uniform.m_num);
					uint8_t len = (uint8_t)bx::strLen(_name) + 1;
					cmdbuf.write(len);
					cmdbuf.write(_name, len);
				}

				++uniform.m_refCount;
				return handle;
			}

			UniformHandle handle = { m_uniformHandle.alloc() };

			if (!isValid(handle))
			{
				BX_TRACE("Failed to allocate uniform handle.");
				return MYGFX_INVALID_HANDLE;
			}

			BX_TRACE("Creating uniform (handle %3d) %s", handle.idx, _name);

			UniformRef& uniform = m_uniformRef[handle.idx];
			uniform.m_name.set(_name);
			uniform.m_refCount = 1;
			uniform.m_type = _type;
			uniform.m_num = _num;

			bool ok = m_uniformHashMap.insert(bx::hash<bx::HashMurmur2A>(_name), handle.idx);
			BX_CHECK(ok, "Uniform already exists (name: %s)!", _name); BX_UNUSED(ok);

			CommandBuffer& cmdbuf = getCommandBuffer(CommandBuffer::CreateUniform);
			cmdbuf.write(handle);
			cmdbuf.write(_type);
			cmdbuf.write(_num);
			uint8_t len = (uint8_t)bx::strLen(_name) + 1;
			cmdbuf.write(len);
			cmdbuf.write(_name, len);

			return handle;
		}

		void getUniformInfo(UniformHandle _handle, UniformInfo& _info);

		void destroyUniform(UniformHandle _handle);

		OcclusionQueryHandle createOcclusionQuery();

		OcclusionQueryResult::Enum getResult(OcclusionQueryHandle _handle, int32_t* _result);

		void destroyOcclusionQuery(OcclusionQueryHandle _handle);

		void requestScreenShot(FrameBufferHandle _handle, const char* _filePath);

		void setPaletteColor(uint8_t _index, const float _rgba[4]);

		void setViewName(ViewId _id, const char* _name);

		void setViewRect(ViewId _id, uint16_t _x, uint16_t _y, uint16_t _width, uint16_t _height)
		{
			m_view[_id].setRect(_x, _y, _width, _height);
		}

		void setViewScissor(ViewId _id, uint16_t _x, uint16_t _y, uint16_t _width, uint16_t _height)
		{
			m_view[_id].setScissor(_x, _y, _width, _height);
		}

		void setViewClear(ViewId _id, uint16_t _flags, uint32_t _rgba, float _depth, uint8_t _stencil)
		{
			BX_CHECK(bx::equal(_depth, bx::clamp(_depth, 0.0f, 1.0f), 0.0001f)
				, "Clear depth value must be between 0.0 and 1.0 (_depth %f)."
				, _depth
			);

			m_view[_id].setClear(_flags, _rgba, _depth, _stencil);
		}

		void setViewClear(ViewId _id, uint16_t _flags, float _depth, uint8_t _stencil, uint8_t _0, uint8_t _1, uint8_t _2, uint8_t _3, uint8_t _4, uint8_t _5, uint8_t _6, uint8_t _7)
		{
			BX_CHECK(bx::equal(_depth, bx::clamp(_depth, 0.0f, 1.0f), 0.0001f)
				, "Clear depth value must be between 0.0 and 1.0 (_depth %f)."
				, _depth
			);

			m_view[_id].setClear(_flags, _depth, _stencil, _0, _1, _2, _3, _4, _5, _6, _7);
		}

		void setViewMode(ViewId _id, ViewMode::Enum _mode)
		{
			m_view[_id].setMode(_mode);
		}

		void setViewFrameBuffer(ViewId _id, FrameBufferHandle _handle)
		{
			MYGFX_CHECK_HANDLE_INVALID_OK("setViewFrameBuffer", m_frameBufferHandle, _handle);
			m_view[_id].setFrameBuffer(_handle);
		}

		void setViewTransform(ViewId _id, const void* _view, const void* _proj, uint8_t _flags, const void* _proj1)
		{
			m_view[_id].setTransform(_view, _proj, _flags, _proj1);
		}

		void resetView(ViewId _id)
		{
			m_view[_id].reset();
		}

		void setViewOrder(ViewId _id, uint16_t _num, const ViewId* _order);

		Encoder* begin(bool _forThread);

		void end(Encoder* _encoder);

		uint32_t frame(bool _capture = false);

		uint32_t getSeqIncr(ViewId _id)
		{
			return bx::atomicFetchAndAdd<uint32_t>(&m_seq[_id], 1);
		}

		void dumpViewStats();
		void freeDynamicBuffers();
		void freeAllHandles(Frame* _frame);
		void frameNoRenderWait();
		void swap();

		// render thread
		void flip();
		RenderFrame::Enum renderFrame(int32_t _msecs = -1);
		void flushTextureUpdateBatch(CommandBuffer& _cmdbuf);
		void rendererExecCommands(CommandBuffer& _cmdbuf);

#if MYGFX_CONFIG_MULTITHREADED
		void apiSemPost()
		{
			if (!m_singleThreaded)
			{
				m_apiSem.post();
			}
		}

		bool apiSemWait(int32_t _msecs = -1)
		{
			if (m_singleThreaded)
			{
				return true;
			}

			MYGFX_PROFILER_SCOPE("bgfx/API thread wait", 0xff2040ff);
			int64_t start = bx::getHPCounter();
			bool ok = m_apiSem.wait(_msecs);
			if (ok)
			{
				m_render->m_waitSubmit = bx::getHPCounter() - start;
				m_submit->m_perfStats.waitSubmit = m_submit->m_waitSubmit;
				return true;
			}

			return false;
		}

		void renderSemPost()
		{
			if (!m_singleThreaded)
			{
				m_renderSem.post();
			}
		}

		void renderSemWait()
		{
			if (!m_singleThreaded)
			{
				MYGFX_PROFILER_SCOPE("bgfx/Render thread wait", 0xff2040ff);
				int64_t start = bx::getHPCounter();
				bool ok = m_renderSem.wait();
				BX_CHECK(ok, "Semaphore wait failed."); BX_UNUSED(ok);
				m_submit->m_waitRender = bx::getHPCounter() - start;
				m_submit->m_perfStats.waitRender = m_submit->m_waitRender;
			}
		}

		void encoderApiWait()
		{
			uint16_t numEncoders = m_encoderHandle->getNumHandles();

			for (uint16_t ii = 1; ii < numEncoders; ++ii)
			{
				m_encoderEndSem.wait();
			}

			for (uint16_t ii = 0; ii < numEncoders; ++ii)
			{
				uint16_t idx = m_encoderHandle->getHandleAt(ii);
				m_encoderStats[ii].cpuTimeBegin = m_encoder[idx].m_cpuTimeBegin;
				m_encoderStats[ii].cpuTimeEnd = m_encoder[idx].m_cpuTimeEnd;
			}

			m_submit->m_perfStats.numEncoders = uint8_t(numEncoders);

			m_encoderHandle->reset();
			uint16_t idx = m_encoderHandle->alloc();
			BX_CHECK(0 == idx, "Internal encoder handle is not 0 (idx %d).", idx); BX_UNUSED(idx);
		}

		bx::Semaphore m_renderSem;
		bx::Semaphore m_apiSem;
		bx::Semaphore m_encoderEndSem;
		bx::Mutex     m_encoderApiLock;
		bx::Mutex     m_resourceApiLock;
		bx::Thread    m_thread;
#else
		void apiSemPost()
		{
		}

		bool apiSemWait(int32_t _msecs = -1)
		{
			BX_UNUSED(_msecs);
			return true;
		}

		void renderSemPost()
		{
		}

		void renderSemWait()
		{
		}

		void encoderApiWait()
		{
			m_encoderStats[0].cpuTimeBegin = m_encoder[0].m_cpuTimeBegin;
			m_encoderStats[0].cpuTimeEnd = m_encoder[0].m_cpuTimeEnd;
			m_submit->m_perfStats.numEncoders = 1;
		}
#endif // BGFX_CONFIG_MULTITHREADED

		EncoderStats* m_encoderStats;
		Encoder*      m_encoder0;
		EncoderImpl*  m_encoder;
		uint32_t      m_numEncoders;
		bx::HandleAlloc* m_encoderHandle;

		Frame  m_frame[1 + (MYGFX_CONFIG_MULTITHREADED ? 1 : 0)];
		Frame* m_render;
		Frame* m_submit;

		uint64_t m_tempKeys[MYGFX_CONFIG_MAX_DRAW_CALLS];
		RenderItemCount m_tempValues[MYGFX_CONFIG_MAX_DRAW_CALLS];

		IndexBuffer  m_indexBuffers[MYGFX_CONFIG_MAX_INDEX_BUFFERS];
		VertexBuffer m_vertexBuffers[MYGFX_CONFIG_MAX_VERTEX_BUFFERS];

		DynamicIndexBuffer  m_dynamicIndexBuffers[MYGFX_CONFIG_MAX_DYNAMIC_INDEX_BUFFERS];
		DynamicVertexBuffer m_dynamicVertexBuffers[MYGFX_CONFIG_MAX_DYNAMIC_VERTEX_BUFFERS];

		uint16_t m_numFreeDynamicIndexBufferHandles;
		uint16_t m_numFreeDynamicVertexBufferHandles;
		uint16_t m_numFreeOcclusionQueryHandles;
		DynamicIndexBufferHandle  m_freeDynamicIndexBufferHandle[MYGFX_CONFIG_MAX_DYNAMIC_INDEX_BUFFERS];
		DynamicVertexBufferHandle m_freeDynamicVertexBufferHandle[MYGFX_CONFIG_MAX_DYNAMIC_VERTEX_BUFFERS];
		OcclusionQueryHandle      m_freeOcclusionQueryHandle[MYGFX_CONFIG_MAX_OCCLUSION_QUERIES];

		NonLocalAllocator m_dynIndexBufferAllocator;
		bx::HandleAllocT<MYGFX_CONFIG_MAX_DYNAMIC_INDEX_BUFFERS> m_dynamicIndexBufferHandle;
		NonLocalAllocator m_dynVertexBufferAllocator;
		bx::HandleAllocT<MYGFX_CONFIG_MAX_DYNAMIC_VERTEX_BUFFERS> m_dynamicVertexBufferHandle;

		bx::HandleAllocT<MYGFX_CONFIG_MAX_INDEX_BUFFERS> m_indexBufferHandle;
		bx::HandleAllocT<MYGFX_CONFIG_MAX_VERTEX_DECLS > m_vertexDeclHandle;

		bx::HandleAllocT<MYGFX_CONFIG_MAX_VERTEX_BUFFERS> m_vertexBufferHandle;
		bx::HandleAllocT<MYGFX_CONFIG_MAX_SHADERS> m_shaderHandle;
		bx::HandleAllocT<MYGFX_CONFIG_MAX_PROGRAMS> m_programHandle;
		bx::HandleAllocT<MYGFX_CONFIG_MAX_TEXTURES> m_textureHandle;
		bx::HandleAllocT<MYGFX_CONFIG_MAX_FRAME_BUFFERS> m_frameBufferHandle;
		bx::HandleAllocT<MYGFX_CONFIG_MAX_UNIFORMS> m_uniformHandle;
		bx::HandleAllocT<MYGFX_CONFIG_MAX_OCCLUSION_QUERIES> m_occlusionQueryHandle;

		struct ShaderRef
		{
			UniformHandle* m_uniforms;
			String   m_name;
			uint32_t m_hashIn;
			uint32_t m_hashOut;
			int16_t  m_refCount;
			uint16_t m_num;
		};

		struct ProgramRef
		{
			ShaderHandle m_vsh;
			ShaderHandle m_fsh;
			int16_t      m_refCount;
		};

		struct UniformRef
		{
			String            m_name;
			UniformType::Enum m_type;
			uint16_t          m_num;
			int16_t           m_refCount;
		};

		struct TextureRef
		{
			void init(
				BackbufferRatio::Enum _ratio
				, TextureFormat::Enum _format
				, uint32_t _storageSize
				, uint8_t _numMips
				, uint16_t _numLayers
				, bool _ptrPending
				, bool _immutable
				, bool _rt
			)
			{
				m_ptr = _ptrPending ? (void*)UINTPTR_MAX : NULL;
				m_storageSize = _storageSize;
				m_refCount = 1;
				m_bbRatio = uint8_t(_ratio);
				m_format = uint8_t(_format);
				m_numMips = _numMips;
				m_numLayers = _numLayers;
				m_owned = false;
				m_immutable = _immutable;
				m_rt = _rt;
			}

			String   m_name;
			void*    m_ptr;
			uint32_t m_storageSize;
			int16_t  m_refCount;
			uint8_t  m_bbRatio;
			uint8_t  m_format;
			uint8_t  m_numMips;
			uint16_t m_numLayers;
			bool     m_owned;
			bool     m_immutable;
			bool     m_rt;
		};

		struct FrameBufferRef
		{
			union un
			{
				TextureHandle m_th[MYGFX_CONFIG_MAX_FRAME_BUFFER_ATTACHMENTS];
				void* m_nwh;
			} un;
			bool m_window;
		};

		typedef bx::HandleHashMapT<MYGFX_CONFIG_MAX_UNIFORMS * 2> UniformHashMap;
		UniformHashMap m_uniformHashMap;
		UniformRef m_uniformRef[MYGFX_CONFIG_MAX_UNIFORMS];

		typedef bx::HandleHashMapT<MYGFX_CONFIG_MAX_SHADERS * 2> ShaderHashMap;
		ShaderHashMap m_shaderHashMap;
		ShaderRef m_shaderRef[MYGFX_CONFIG_MAX_SHADERS];

		typedef bx::HandleHashMapT<MYGFX_CONFIG_MAX_PROGRAMS * 2> ProgramHashMap;
		ProgramHashMap m_programHashMap;
		ProgramRef m_programRef[MYGFX_CONFIG_MAX_PROGRAMS];

		TextureRef m_textureRef[MYGFX_CONFIG_MAX_TEXTURES];
		FrameBufferRef m_frameBufferRef[MYGFX_CONFIG_MAX_FRAME_BUFFERS];
		VertexDeclRef m_declRef;

		ViewId m_viewRemap[MYGFX_CONFIG_MAX_VIEWS];
		uint32_t m_seq[MYGFX_CONFIG_MAX_VIEWS];
		View m_view[MYGFX_CONFIG_MAX_VIEWS];

		float m_clearColor[MYGFX_CONFIG_MAX_COLOR_PALETTE][4];

		uint8_t m_colorPaletteDirty;

		Init     m_init;
		int64_t  m_frameTimeLast;
		uint32_t m_frames;
		uint32_t m_debug;

		int64_t m_rtMemoryUsed;
		int64_t m_textureMemoryUsed;

		TextVideoMemBlitter m_textVideoMemBlitter;
		ClearQuad m_clearQuad;

		RendererContextI* m_renderCtx;
		RendererContextI* m_renderMain;
		RendererContextI* m_renderNoop;

		bool m_rendererInitialized;
		bool m_exit;
		bool m_flipAfterRender;
		bool m_singleThreaded;
		bool m_flipped;

		typedef UpdateBatchT<256> TextureUpdateBatch;
		BX_ALIGN_DECL_CACHE_LINE(TextureUpdateBatch m_textureUpdateBatch);
	};

}