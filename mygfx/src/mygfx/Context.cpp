#include "mygfx_p.h"

namespace mygfx
{
	Context::Context() : m_render(&m_frame[0])
		, m_submit(&m_frame[MYGFX_CONFIG_MULTITHREADED ? 1 : 0])
		, m_numFreeDynamicIndexBufferHandles(0)
		, m_numFreeDynamicVertexBufferHandles(0)
		, m_numFreeOcclusionQueryHandles(0)
		, m_colorPaletteDirty(0)
		, m_frames(0)
		, m_debug(MYGFX_DEBUG_NONE)
		, m_rtMemoryUsed(0)
		, m_textureMemoryUsed(0)
		, m_renderCtx(NULL)
		, m_renderMain(NULL)
		, m_renderNoop(NULL)
		, m_rendererInitialized(false)
		, m_exit(false)
		, m_flipAfterRender(false)
		, m_singleThreaded(false)
	{

	}

	static int32_t renderThread(bx::Thread* /*_self*/, void* /*_userData*/)
	{
		BX_TRACE("render thread start");
		MYGFX_PROFILER_SET_CURRENT_THREAD_NAME("bgfx - Render Thread");
		while (RenderFrame::Exiting != mygfx::renderFrame()) {};
		BX_TRACE("render thread exit");
		return bx::kExitSuccess;
	}

	mygfx::CommandBuffer& Context::getCommandBuffer(CommandBuffer::Enum _cmd)
	{
		CommandBuffer& cmdbuf = _cmd < CommandBuffer::End ? m_submit->m_cmdPre : m_submit->m_cmdPost;
		uint8_t cmd = (uint8_t)_cmd;
		cmdbuf.write(cmd);
		return cmdbuf;
	}

	void Context::reset(uint32_t _width, uint32_t _height, uint32_t _flags, TextureFormat::Enum _format)
	{
		BX_WARN(g_caps.limits.maxTextureSize >= _width
			&&  g_caps.limits.maxTextureSize >= _height
			, "Frame buffer resolution width or height can't be larger than limits.maxTextureSize %d (width %d, height %d)."
			, g_caps.limits.maxTextureSize
			, _width
			, _height
		);
		m_init.resolution.format = TextureFormat::Count != _format ? _format : m_init.resolution.format;
		m_init.resolution.width = bx::clamp(_width, 1u, g_caps.limits.maxTextureSize);
		m_init.resolution.height = bx::clamp(_height, 1u, g_caps.limits.maxTextureSize);
		m_init.resolution.reset = 0
			| _flags
			| (g_platformDataChangedSinceReset ? MYGFX_RESET_INTERNAL_FORCE : 0)
			;
		g_platformDataChangedSinceReset = false;

		m_flipAfterRender = !!(_flags & MYGFX_RESET_FLIP_AFTER_RENDER);

		for (uint32_t ii = 0; ii < MYGFX_CONFIG_MAX_VIEWS; ++ii)
		{
			m_view[ii].setFrameBuffer(MYGFX_INVALID_HANDLE);
		}

		for (uint16_t ii = 0, num = m_textureHandle.getNumHandles(); ii < num; ++ii)
		{
			uint16_t textureIdx = m_textureHandle.getHandleAt(ii);
			const TextureRef& textureRef = m_textureRef[textureIdx];
			if (BackbufferRatio::Count != textureRef.m_bbRatio)
			{
				TextureHandle handle = { textureIdx };
				resizeTexture(handle
					, uint16_t(m_init.resolution.width)
					, uint16_t(m_init.resolution.height)
					, textureRef.m_numMips
					, textureRef.m_numLayers
				);
				m_init.resolution.reset |= MYGFX_RESET_INTERNAL_FORCE;
			}
		}
	}

	void Context::dbgTextClear(uint8_t _attr, bool _small)
	{
		MYGFX_MUTEX_SCOPE(m_resourceApiLock);

		m_submit->m_textVideoMem->resize(_small, (uint16_t)m_init.resolution.width, (uint16_t)m_init.resolution.height);
		m_submit->m_textVideoMem->clear(_attr);
	}

	void Context::dbgTextPrintfVargs(uint16_t _x, uint16_t _y, uint8_t _attr, const char* _format, va_list _argList)
	{
		MYGFX_MUTEX_SCOPE(m_resourceApiLock);

		m_submit->m_textVideoMem->printfVargs(_x, _y, _attr, _format, _argList);
	}

	void Context::dbgTextImage(uint16_t _x, uint16_t _y, uint16_t _width, uint16_t _height, const void* _data, uint16_t _pitch)
	{
		MYGFX_MUTEX_SCOPE(m_resourceApiLock);

		m_submit->m_textVideoMem->image(_x, _y, _width, _height, _data, _pitch);
	}

	const mygfx::Stats* Context::getPerfStats()
	{
		MYGFX_MUTEX_SCOPE(m_resourceApiLock);

		Stats& stats = m_submit->m_perfStats;
		const Resolution& resolution = m_submit->m_resolution;
		stats.width = uint16_t(resolution.width);
		stats.height = uint16_t(resolution.height);
		const TextVideoMem* tvm = m_submit->m_textVideoMem;
		stats.textWidth = tvm->m_width;
		stats.textHeight = tvm->m_height;
		stats.encoderStats = m_encoderStats;

		stats.numDynamicIndexBuffers = m_dynamicIndexBufferHandle.getNumHandles();
		stats.numDynamicVertexBuffers = m_dynamicVertexBufferHandle.getNumHandles();
		stats.numFrameBuffers = m_frameBufferHandle.getNumHandles();
		stats.numIndexBuffers = m_indexBufferHandle.getNumHandles();
		stats.numOcclusionQueries = m_occlusionQueryHandle.getNumHandles();
		stats.numPrograms = m_programHandle.getNumHandles();
		stats.numShaders = m_shaderHandle.getNumHandles();
		stats.numTextures = m_textureHandle.getNumHandles();
		stats.numUniforms = m_uniformHandle.getNumHandles();
		stats.numVertexBuffers = m_vertexBufferHandle.getNumHandles();
		stats.numVertexDecls = m_vertexDeclHandle.getNumHandles();

		stats.textureMemoryUsed = m_textureMemoryUsed;
		stats.rtMemoryUsed = m_rtMemoryUsed;

		return &stats;
	}

	mygfx::IndexBufferHandle Context::createIndexBuffer(const Memory* _mem, uint16_t _flags)
	{
		MYGFX_MUTEX_SCOPE(m_resourceApiLock);

		IndexBufferHandle handle = { m_indexBufferHandle.alloc() };

		BX_WARN(isValid(handle), "Failed to allocate index buffer handle.");
		if (isValid(handle))
		{
			IndexBuffer& ib = m_indexBuffers[handle.idx];
			ib.m_size = _mem->size;

			CommandBuffer& cmdbuf = getCommandBuffer(CommandBuffer::CreateIndexBuffer);
			cmdbuf.write(handle);
			cmdbuf.write(_mem);
			cmdbuf.write(_flags);
		}
		else
		{
			release(_mem);
		}

		return handle;
	}

	void Context::destroyIndexBuffer(IndexBufferHandle _handle)
	{
		MYGFX_MUTEX_SCOPE(m_resourceApiLock);

		MYGFX_CHECK_HANDLE("destroyIndexBuffer", m_indexBufferHandle, _handle);
		bool ok = m_submit->free(_handle); BX_UNUSED(ok);
		BX_CHECK(ok, "Index buffer handle %d is already destroyed!", _handle.idx);

		CommandBuffer& cmdbuf = getCommandBuffer(CommandBuffer::DestroyIndexBuffer);
		cmdbuf.write(_handle);
	}

	mygfx::VertexDeclHandle Context::findVertexDecl(const VertexDecl& _decl)
	{
		VertexDeclHandle declHandle = m_declRef.find(_decl.m_hash);

		if (!isValid(declHandle))
		{
			declHandle.idx = m_vertexDeclHandle.alloc();
			if (!isValid(declHandle))
			{
				return declHandle;
			}

			CommandBuffer& cmdbuf = getCommandBuffer(CommandBuffer::CreateVertexDecl);
			cmdbuf.write(declHandle);
			cmdbuf.write(_decl);
		}

		return declHandle;
	}

	mygfx::VertexBufferHandle Context::createVertexBuffer(const Memory* _mem, const VertexDecl& _decl, uint16_t _flags)
	{
		MYGFX_MUTEX_SCOPE(m_resourceApiLock);

		VertexBufferHandle handle = { m_vertexBufferHandle.alloc() };

		if (isValid(handle))
		{
			VertexDeclHandle declHandle = findVertexDecl(_decl);
			if (!isValid(declHandle))
			{
				BX_TRACE("WARNING: Failed to allocate vertex decl handle (BGFX_CONFIG_MAX_VERTEX_DECLS, max: %d).", MYGFX_CONFIG_MAX_VERTEX_DECLS);
				m_vertexBufferHandle.free(handle.idx);
				return MYGFX_INVALID_HANDLE;
			}

			m_declRef.add(handle, declHandle, _decl.m_hash);

			VertexBuffer& vb = m_vertexBuffers[handle.idx];
			vb.m_size = _mem->size;
			vb.m_stride = _decl.m_stride;

			CommandBuffer& cmdbuf = getCommandBuffer(CommandBuffer::CreateVertexBuffer);
			cmdbuf.write(handle);
			cmdbuf.write(_mem);
			cmdbuf.write(declHandle);
			cmdbuf.write(_flags);

			return handle;
		}

		BX_TRACE("WARNING: Failed to allocate vertex buffer handle (BGFX_CONFIG_MAX_VERTEX_BUFFERS, max: %d).", BGFX_CONFIG_MAX_VERTEX_BUFFERS);
		release(_mem);

		return MYGFX_INVALID_HANDLE;
	}

	void Context::destroyVertexBuffer(VertexBufferHandle _handle)
	{
		MYGFX_MUTEX_SCOPE(m_resourceApiLock);

		MYGFX_CHECK_HANDLE("destroyVertexBuffer", m_vertexBufferHandle, _handle);
		bool ok = m_submit->free(_handle); BX_UNUSED(ok);
		BX_CHECK(ok, "Vertex buffer handle %d is already destroyed!", _handle.idx);

		CommandBuffer& cmdbuf = getCommandBuffer(CommandBuffer::DestroyVertexBuffer);
		cmdbuf.write(_handle);
	}

	void Context::destroyVertexBufferInternal(VertexBufferHandle _handle)
	{
		VertexDeclHandle declHandle = m_declRef.release(_handle);
		if (isValid(declHandle))
		{
			CommandBuffer& cmdbuf = getCommandBuffer(CommandBuffer::DestroyVertexDecl);
			cmdbuf.write(declHandle);
			m_render->free(declHandle);
		}

		m_vertexBufferHandle.free(_handle.idx);
	}

	uint64_t Context::allocDynamicIndexBuffer(uint32_t _size, uint16_t _flags)
	{
		uint64_t ptr = m_dynIndexBufferAllocator.alloc(_size);
		if (ptr == NonLocalAllocator::kInvalidBlock)
		{
			IndexBufferHandle indexBufferHandle = { m_indexBufferHandle.alloc() };
			BX_WARN(isValid(indexBufferHandle), "Failed to allocate index buffer handle.");
			if (!isValid(indexBufferHandle))
			{
				return NonLocalAllocator::kInvalidBlock;
			}

			const uint32_t allocSize = bx::max<uint32_t>(MYGFX_CONFIG_DYNAMIC_INDEX_BUFFER_SIZE, _size);

			IndexBuffer& ib = m_indexBuffers[indexBufferHandle.idx];
			ib.m_size = allocSize;

			CommandBuffer& cmdbuf = getCommandBuffer(CommandBuffer::CreateDynamicIndexBuffer);
			cmdbuf.write(indexBufferHandle);
			cmdbuf.write(allocSize);
			cmdbuf.write(_flags);

			m_dynIndexBufferAllocator.add(uint64_t(indexBufferHandle.idx) << 32, allocSize);
			ptr = m_dynIndexBufferAllocator.alloc(_size);
		}

		return ptr;
	}

	mygfx::DynamicIndexBufferHandle Context::createDynamicIndexBuffer(uint32_t _num, uint16_t _flags)
	{
		MYGFX_MUTEX_SCOPE(m_resourceApiLock);

		DynamicIndexBufferHandle handle = { m_dynamicIndexBufferHandle.alloc() };
		BX_WARN(isValid(handle), "Failed to allocate dynamic index buffer handle.");
		if (!isValid(handle))
		{
			return handle;
		}

		const uint32_t indexSize = 0 == (_flags & MYGFX_BUFFER_INDEX32) ? 2 : 4;
		uint32_t size = BX_ALIGN_16(_num*indexSize);

		uint64_t ptr = 0;
		if (0 != (_flags & MYGFX_BUFFER_COMPUTE_READ_WRITE))
		{
			IndexBufferHandle indexBufferHandle = { m_indexBufferHandle.alloc() };
			if (!isValid(indexBufferHandle))
			{
				m_dynamicIndexBufferHandle.free(handle.idx);
				return MYGFX_INVALID_HANDLE;
			}

			IndexBuffer& ib = m_indexBuffers[indexBufferHandle.idx];
			ib.m_size = size;

			CommandBuffer& cmdbuf = getCommandBuffer(CommandBuffer::CreateDynamicIndexBuffer);
			cmdbuf.write(indexBufferHandle);
			cmdbuf.write(size);
			cmdbuf.write(_flags);

			ptr = uint64_t(indexBufferHandle.idx) << 32;
		}
		else
		{
			ptr = allocDynamicIndexBuffer(size, _flags);
			if (ptr == NonLocalAllocator::kInvalidBlock)
			{
				m_dynamicIndexBufferHandle.free(handle.idx);
				return MYGFX_INVALID_HANDLE;
			}
		}

		DynamicIndexBuffer& dib = m_dynamicIndexBuffers[handle.idx];
		dib.m_handle.idx = uint16_t(ptr >> 32);
		dib.m_offset = uint32_t(ptr);
		dib.m_size = _num * indexSize;
		dib.m_startIndex = bx::strideAlign(dib.m_offset, indexSize) / indexSize;
		dib.m_flags = _flags;

		return handle;
	}

	mygfx::DynamicIndexBufferHandle Context::createDynamicIndexBuffer(const Memory* _mem, uint16_t _flags)
	{
		MYGFX_MUTEX_SCOPE(m_resourceApiLock);

		BX_CHECK(0 == (_flags &  MYGFX_BUFFER_COMPUTE_READ_WRITE), "Cannot initialize compute buffer from CPU.");
		const uint32_t indexSize = 0 == (_flags & MYGFX_BUFFER_INDEX32) ? 2 : 4;
		DynamicIndexBufferHandle handle = createDynamicIndexBuffer(_mem->size / indexSize, _flags);

		if (!isValid(handle))
		{
			release(_mem);
			return MYGFX_INVALID_HANDLE;
		}

		update(handle, 0, _mem);

		return handle;
	}

	mygfx::DynamicVertexBufferHandle Context::createDynamicVertexBuffer(uint32_t _num, const VertexDecl& _decl, uint16_t _flags)
	{
		MYGFX_MUTEX_SCOPE(m_resourceApiLock);

		VertexDeclHandle declHandle = findVertexDecl(_decl);
		if (!isValid(declHandle))
		{
			BX_TRACE("WARNING: Failed to allocate vertex decl handle (BGFX_CONFIG_MAX_VERTEX_DECLS, max: %d).", MYGFX_CONFIG_MAX_VERTEX_DECLS);
			return MYGFX_INVALID_HANDLE;
		}

		DynamicVertexBufferHandle handle = { m_dynamicVertexBufferHandle.alloc() };
		if (!isValid(handle))
		{
			BX_TRACE("WARNING: Failed to allocate dynamic vertex buffer handle (BGFX_CONFIG_MAX_DYNAMIC_VERTEX_BUFFERS, max: %d).", BGFX_CONFIG_MAX_DYNAMIC_VERTEX_BUFFERS);
			return MYGFX_INVALID_HANDLE;
		}

		uint32_t size = bx::strideAlign16(_num*_decl.m_stride, _decl.m_stride);

		uint64_t ptr = 0;
		if (0 != (_flags & MYGFX_BUFFER_COMPUTE_READ_WRITE))
		{
			VertexBufferHandle vertexBufferHandle = { m_vertexBufferHandle.alloc() };
			if (!isValid(vertexBufferHandle))
			{
				m_dynamicVertexBufferHandle.free(handle.idx);
				BX_TRACE("WARNING: Failed to allocate vertex buffer handle (BGFX_CONFIG_MAX_VERTEX_BUFFERS, max: %d).", BGFX_CONFIG_MAX_VERTEX_BUFFERS);
				return MYGFX_INVALID_HANDLE;
			}

			VertexBuffer& vb = m_vertexBuffers[vertexBufferHandle.idx];
			vb.m_size = size;
			vb.m_stride = 0;

			CommandBuffer& cmdbuf = getCommandBuffer(CommandBuffer::CreateDynamicVertexBuffer);
			cmdbuf.write(vertexBufferHandle);
			cmdbuf.write(size);
			cmdbuf.write(_flags);

			ptr = uint64_t(vertexBufferHandle.idx) << 32;
		}
		else
		{
			ptr = allocDynamicVertexBuffer(size, _flags);
			if (ptr == NonLocalAllocator::kInvalidBlock)
			{
				m_dynamicVertexBufferHandle.free(handle.idx);
				return MYGFX_INVALID_HANDLE;
			}
		}

		DynamicVertexBuffer& dvb = m_dynamicVertexBuffers[handle.idx];
		dvb.m_handle.idx = uint16_t(ptr >> 32);
		dvb.m_offset = uint32_t(ptr);
		dvb.m_size = _num * _decl.m_stride;
		dvb.m_startVertex = bx::strideAlign(dvb.m_offset, _decl.m_stride) / _decl.m_stride;
		dvb.m_numVertices = _num;
		dvb.m_stride = _decl.m_stride;
		dvb.m_decl = declHandle;
		dvb.m_flags = _flags;
		m_declRef.add(handle, declHandle, _decl.m_hash);

		return handle;
	}

	mygfx::DynamicVertexBufferHandle Context::createDynamicVertexBuffer(const Memory* _mem, const VertexDecl& _decl, uint16_t _flags)
	{
		MYGFX_MUTEX_SCOPE(m_resourceApiLock);

		uint32_t numVertices = _mem->size / _decl.m_stride;
		DynamicVertexBufferHandle handle = createDynamicVertexBuffer(numVertices, _decl, _flags);

		if (!isValid(handle))
		{
			release(_mem);
			return MYGFX_INVALID_HANDLE;
		}

		update(handle, 0, _mem);

		return handle;
	}

	void Context::update(DynamicIndexBufferHandle _handle, uint32_t _startIndex, const Memory* _mem)
	{
		MYGFX_MUTEX_SCOPE(m_resourceApiLock);

		MYGFX_CHECK_HANDLE("updateDynamicIndexBuffer", m_dynamicIndexBufferHandle, _handle);

		DynamicIndexBuffer& dib = m_dynamicIndexBuffers[_handle.idx];
		BX_CHECK(0 == (dib.m_flags &  MYGFX_BUFFER_COMPUTE_WRITE), "Can't update GPU buffer from CPU.");
		const uint32_t indexSize = 0 == (dib.m_flags & MYGFX_BUFFER_INDEX32) ? 2 : 4;

		if (dib.m_size < _mem->size
			&& 0 != (dib.m_flags & MYGFX_BUFFER_ALLOW_RESIZE))
		{
			m_dynIndexBufferAllocator.free(uint64_t(dib.m_handle.idx) << 32 | dib.m_offset);
			m_dynIndexBufferAllocator.compact();

			uint64_t ptr = allocDynamicIndexBuffer(_mem->size, dib.m_flags);
			dib.m_handle.idx = uint16_t(ptr >> 32);
			dib.m_offset = uint32_t(ptr);
			dib.m_size = _mem->size;
			dib.m_startIndex = bx::strideAlign(dib.m_offset, indexSize) / indexSize;
		}

		const uint32_t offset = (dib.m_startIndex + _startIndex)*indexSize;
		const uint32_t size = bx::min<uint32_t>(offset
			+ bx::min(bx::uint32_satsub(dib.m_size, _startIndex*indexSize), _mem->size)
			, m_indexBuffers[dib.m_handle.idx].m_size) - offset
			;
		BX_CHECK(_mem->size <= size, "Truncating dynamic index buffer update (size %d, mem size %d)."
			, size
			, _mem->size
		);
		CommandBuffer& cmdbuf = getCommandBuffer(CommandBuffer::UpdateDynamicIndexBuffer);
		cmdbuf.write(dib.m_handle);
		cmdbuf.write(offset);
		cmdbuf.write(size);
		cmdbuf.write(_mem);
	}

	void Context::update(DynamicVertexBufferHandle _handle, uint32_t _startVertex, const Memory* _mem)
	{
		MYGFX_MUTEX_SCOPE(m_resourceApiLock);

		MYGFX_CHECK_HANDLE("updateDynamicVertexBuffer", m_dynamicVertexBufferHandle, _handle);

		DynamicVertexBuffer& dvb = m_dynamicVertexBuffers[_handle.idx];
		BX_CHECK(0 == (dvb.m_flags &  MYGFX_BUFFER_COMPUTE_WRITE), "Can't update GPU write buffer from CPU.");

		if (dvb.m_size < _mem->size
			&& 0 != (dvb.m_flags & MYGFX_BUFFER_ALLOW_RESIZE))
		{
			m_dynVertexBufferAllocator.free(uint64_t(dvb.m_handle.idx) << 32 | dvb.m_offset);
			m_dynVertexBufferAllocator.compact();

			uint64_t ptr = allocDynamicVertexBuffer(_mem->size, dvb.m_flags);
			dvb.m_handle.idx = uint16_t(ptr >> 32);
			dvb.m_offset = uint32_t(ptr);
			dvb.m_size = _mem->size;
			dvb.m_numVertices = dvb.m_size / dvb.m_stride;
			dvb.m_startVertex = bx::strideAlign(dvb.m_offset, dvb.m_stride) / dvb.m_stride;
		}

		const uint32_t offset = (dvb.m_startVertex + _startVertex)*dvb.m_stride;
		const uint32_t size = bx::min<uint32_t>(offset
			+ bx::min(bx::uint32_satsub(dvb.m_size, _startVertex*dvb.m_stride), _mem->size)
			, m_vertexBuffers[dvb.m_handle.idx].m_size) - offset
			;
		BX_CHECK(_mem->size <= size, "Truncating dynamic vertex buffer update (size %d, mem size %d)."
			, size
			, _mem->size
		);

		CommandBuffer& cmdbuf = getCommandBuffer(CommandBuffer::UpdateDynamicVertexBuffer);
		cmdbuf.write(dvb.m_handle);
		cmdbuf.write(offset);
		cmdbuf.write(size);
		cmdbuf.write(_mem);
	}

	void Context::destroyDynamicVertexBuffer(DynamicVertexBufferHandle _handle)
	{
		MYGFX_MUTEX_SCOPE(m_resourceApiLock);

		MYGFX_CHECK_HANDLE("destroyDynamicVertexBuffer", m_dynamicVertexBufferHandle, _handle);

		m_freeDynamicVertexBufferHandle[m_numFreeDynamicVertexBufferHandles++] = _handle;
	}

	void Context::destroyDynamicVertexBufferInternal(DynamicVertexBufferHandle _handle)
	{
		VertexDeclHandle declHandle = m_declRef.release(_handle);
		MYGFX_CHECK_HANDLE_INVALID_OK("destroyDynamicVertexBufferInternal", m_vertexDeclHandle, declHandle);

		if (isValid(declHandle))
		{
			CommandBuffer& cmdbuf = getCommandBuffer(CommandBuffer::DestroyVertexDecl);
			cmdbuf.write(declHandle);
			m_render->free(declHandle);
		}

		DynamicVertexBuffer& dvb = m_dynamicVertexBuffers[_handle.idx];

		if (0 != (dvb.m_flags & MYGFX_BUFFER_COMPUTE_READ_WRITE))
		{
			destroyVertexBuffer(dvb.m_handle);
		}
		else
		{
			m_dynVertexBufferAllocator.free(uint64_t(dvb.m_handle.idx) << 32 | dvb.m_offset);
			if (m_dynVertexBufferAllocator.compact())
			{
				for (uint64_t ptr = m_dynVertexBufferAllocator.remove(); 0 != ptr; ptr = m_dynVertexBufferAllocator.remove())
				{
					VertexBufferHandle handle = { uint16_t(ptr >> 32) };
					destroyVertexBuffer(handle);
				}
			}
		}

		m_dynamicVertexBufferHandle.free(_handle.idx);
	}

	uint32_t Context::getAvailTransientIndexBuffer(uint32_t _num)
	{
		MYGFX_MUTEX_SCOPE(m_resourceApiLock);

		return m_submit->getAvailTransientIndexBuffer(_num);
	}

	uint32_t Context::getAvailTransientVertexBuffer(uint32_t _num, uint16_t _stride)
	{
		MYGFX_MUTEX_SCOPE(m_resourceApiLock);

		return m_submit->getAvailTransientVertexBuffer(_num, _stride);
	}

	mygfx::TransientIndexBuffer* Context::createTransientIndexBuffer(uint32_t _size)
	{
		TransientIndexBuffer* tib = NULL;

		IndexBufferHandle handle = { m_indexBufferHandle.alloc() };
		BX_WARN(isValid(handle), "Failed to allocate transient index buffer handle.");
		if (isValid(handle))
		{
			CommandBuffer& cmdbuf = getCommandBuffer(CommandBuffer::CreateDynamicIndexBuffer);
			cmdbuf.write(handle);
			cmdbuf.write(_size);
			uint16_t flags = MYGFX_BUFFER_NONE;
			cmdbuf.write(flags);

			const uint32_t size = BX_ALIGN_16(sizeof(TransientIndexBuffer)) + BX_ALIGN_16(_size);
			tib = (TransientIndexBuffer*)BX_ALIGNED_ALLOC(g_allocator, size, 16);
			tib->data = (uint8_t *)tib + BX_ALIGN_16(sizeof(TransientIndexBuffer));
			tib->size = _size;
			tib->handle = handle;
		}

		return tib;
	}

	void Context::destroyTransientIndexBuffer(TransientIndexBuffer* _tib)
	{
		CommandBuffer& cmdbuf = getCommandBuffer(CommandBuffer::DestroyDynamicIndexBuffer);
		cmdbuf.write(_tib->handle);

		m_submit->free(_tib->handle);
		BX_ALIGNED_FREE(g_allocator, _tib, 16);
	}

	void Context::allocTransientIndexBuffer(TransientIndexBuffer* _tib, uint32_t _num)
	{
		MYGFX_MUTEX_SCOPE(m_resourceApiLock);

		uint32_t offset = m_submit->allocTransientIndexBuffer(_num);

		TransientIndexBuffer& tib = *m_submit->m_transientIb;

		_tib->data = &tib.data[offset];
		_tib->size = _num * 2;
		_tib->handle = tib.handle;
		_tib->startIndex = bx::strideAlign(offset, 2) / 2;
	}

	mygfx::TransientVertexBuffer* Context::createTransientVertexBuffer(uint32_t _size, const VertexDecl* _decl /*= NULL*/)
	{
		TransientVertexBuffer* tvb = NULL;

		VertexBufferHandle handle = { m_vertexBufferHandle.alloc() };

		BX_WARN(isValid(handle), "Failed to allocate transient vertex buffer handle.");
		if (isValid(handle))
		{
			uint16_t stride = 0;
			VertexDeclHandle declHandle = MYGFX_INVALID_HANDLE;

			if (NULL != _decl)
			{
				declHandle = findVertexDecl(*_decl);
				m_declRef.add(handle, declHandle, _decl->m_hash);

				stride = _decl->m_stride;
			}

			CommandBuffer& cmdbuf = getCommandBuffer(CommandBuffer::CreateDynamicVertexBuffer);
			cmdbuf.write(handle);
			cmdbuf.write(_size);
			uint16_t flags = MYGFX_BUFFER_NONE;
			cmdbuf.write(flags);

			const uint32_t size = BX_ALIGN_16(sizeof(TransientVertexBuffer)) + BX_ALIGN_16(_size);
			tvb = (TransientVertexBuffer*)BX_ALIGNED_ALLOC(g_allocator, size, 16);
			tvb->data = (uint8_t *)tvb + BX_ALIGN_16(sizeof(TransientVertexBuffer));
			tvb->size = _size;
			tvb->startVertex = 0;
			tvb->stride = stride;
			tvb->handle = handle;
			tvb->decl = declHandle;
		}

		return tvb;
	}

	void Context::destroyTransientVertexBuffer(TransientVertexBuffer* _tvb)
	{
		CommandBuffer& cmdbuf = getCommandBuffer(CommandBuffer::DestroyDynamicVertexBuffer);
		cmdbuf.write(_tvb->handle);

		m_submit->free(_tvb->handle);
		BX_ALIGNED_FREE(g_allocator, _tvb, 16);
	}

	void Context::allocTransientVertexBuffer(TransientVertexBuffer* _tvb, uint32_t _num, const VertexDecl& _decl)
	{
		MYGFX_MUTEX_SCOPE(m_resourceApiLock);

		VertexDeclHandle declHandle = m_declRef.find(_decl.m_hash);

		TransientVertexBuffer& dvb = *m_submit->m_transientVb;

		if (!isValid(declHandle))
		{
			VertexDeclHandle temp = { m_vertexDeclHandle.alloc() };
			declHandle = temp;
			CommandBuffer& cmdbuf = getCommandBuffer(CommandBuffer::CreateVertexDecl);
			cmdbuf.write(declHandle);
			cmdbuf.write(_decl);
			m_declRef.add(declHandle, _decl.m_hash);
		}

		uint32_t offset = m_submit->allocTransientVertexBuffer(_num, _decl.m_stride);

		_tvb->data = &dvb.data[offset];
		_tvb->size = _num * _decl.m_stride;
		_tvb->startVertex = bx::strideAlign(offset, _decl.m_stride) / _decl.m_stride;
		_tvb->stride = _decl.m_stride;
		_tvb->handle = dvb.handle;
		_tvb->decl = declHandle;
	}

	mygfx::ShaderHandle Context::createShader(const Memory* _mem)
	{
		MYGFX_MUTEX_SCOPE(m_resourceApiLock);

		bx::MemoryReader reader(_mem->data, _mem->size);

		bx::Error err;

		uint32_t magic;
		bx::read(&reader, magic, &err);

		if (!err.isOk())
		{
			BX_TRACE("Couldn't read shader signature!");
			release(_mem);
			return MYGFX_INVALID_HANDLE;
		}

		if (!isShaderBin(magic))
		{
			BX_TRACE("Invalid shader signature! %c%c%c%d."
				, ((uint8_t*)&magic)[0]
				, ((uint8_t*)&magic)[1]
				, ((uint8_t*)&magic)[2]
				, ((uint8_t*)&magic)[3]
			);
			release(_mem);
			return MYGFX_INVALID_HANDLE;
		}

		if (isShaderType(magic, 'C')
			&& 0 == (g_caps.supported & MYGFX_CAPS_COMPUTE))
		{
			BX_TRACE("Creating compute shader but compute is not supported!");
			release(_mem);
			return MYGFX_INVALID_HANDLE;
		}

		if ((isShaderType(magic, 'C') && isShaderVerLess(magic, 3))
			|| (isShaderType(magic, 'F') && isShaderVerLess(magic, 5))
			|| (isShaderType(magic, 'V') && isShaderVerLess(magic, 5)))
		{
			BX_TRACE("Unsupported shader binary version.");
			release(_mem);
			return MYGFX_INVALID_HANDLE;
		}

		const uint32_t shaderHash = bx::hash<bx::HashMurmur2A>(_mem->data, _mem->size);
		const uint16_t idx = m_shaderHashMap.find(shaderHash);
		if (kInvalidHandle != idx)
		{
			ShaderHandle handle = { idx };
			shaderIncRef(handle);
			release(_mem);
			return handle;
		}

		uint32_t hashIn;
		bx::read(&reader, hashIn, &err);

		uint32_t hashOut;

		if (isShaderVerLess(magic, 6))
		{
			hashOut = hashIn;
		}
		else
		{
			bx::read(&reader, hashOut, &err);
		}

		uint16_t count;
		bx::read(&reader, count, &err);

		if (!err.isOk())
		{
			BX_TRACE("Corrupted shader binary!");
			release(_mem);
			return MYGFX_INVALID_HANDLE;
		}

		ShaderHandle handle = { m_shaderHandle.alloc() };

		if (!isValid(handle))
		{
			BX_TRACE("Failed to allocate shader handle.");
			release(_mem);
			return MYGFX_INVALID_HANDLE;
		}

		bool ok = m_shaderHashMap.insert(shaderHash, handle.idx);
		BX_CHECK(ok, "Shader already exists!"); BX_UNUSED(ok);

		ShaderRef& sr = m_shaderRef[handle.idx];
		sr.m_refCount = 1;
		sr.m_hashIn = hashIn;
		sr.m_hashOut = hashOut;
		sr.m_num = 0;
		sr.m_uniforms = NULL;

		UniformHandle* uniforms = (UniformHandle*)alloca(count * sizeof(UniformHandle));

		for (uint32_t ii = 0; ii < count; ++ii)
		{
			uint8_t nameSize = 0;
			bx::read(&reader, nameSize, &err);

			char name[256];
			bx::read(&reader, &name, nameSize, &err);
			name[nameSize] = '\0';

			uint8_t type = 0;
			bx::read(&reader, type, &err);
			type &= ~MYGFX_UNIFORM_MASK;

			uint8_t num;
			bx::read(&reader, num, &err);

			uint16_t regIndex;
			bx::read(&reader, regIndex, &err);

			uint16_t regCount;
			bx::read(&reader, regCount, &err);

			PredefinedUniform::Enum predefined = nameToPredefinedUniformEnum(name);
			if (PredefinedUniform::Count == predefined)
			{
				uniforms[sr.m_num] = createUniform(name, UniformType::Enum(type), regCount);
				sr.m_num++;
			}
		}

		if (0 != sr.m_num)
		{
			uint32_t size = sr.m_num * sizeof(UniformHandle);
			sr.m_uniforms = (UniformHandle*)BX_ALLOC(g_allocator, size);
			bx::memCopy(sr.m_uniforms, uniforms, size);
		}

		CommandBuffer& cmdbuf = getCommandBuffer(CommandBuffer::CreateShader);
		cmdbuf.write(handle);
		cmdbuf.write(_mem);

		return handle;
	}

	uint16_t Context::getShaderUniforms(ShaderHandle _handle, UniformHandle* _uniforms, uint16_t _max)
	{
		MYGFX_MUTEX_SCOPE(m_resourceApiLock);

		if (!isValid(_handle))
		{
			BX_WARN(false, "Passing invalid shader handle to bgfx::getShaderUniforms.");
			return 0;
		}

		ShaderRef& sr = m_shaderRef[_handle.idx];
		if (NULL != _uniforms)
		{
			bx::memCopy(_uniforms, sr.m_uniforms, bx::min<uint16_t>(_max, sr.m_num) * sizeof(UniformHandle));
		}

		return sr.m_num;
	}

	void Context::setName(Handle _handle, const bx::StringView& _name)
	{
		CommandBuffer& cmdbuf = getCommandBuffer(CommandBuffer::SetName);
		cmdbuf.write(_handle);
		uint16_t len = uint16_t(_name.getLength() + 1);
		cmdbuf.write(len);
		cmdbuf.write(_name.getPtr(), len - 1);
		cmdbuf.write('\0');
	}

	void Context::setName(ShaderHandle _handle, const bx::StringView& _name)
	{
		MYGFX_MUTEX_SCOPE(m_resourceApiLock);

		MYGFX_CHECK_HANDLE("setName", m_shaderHandle, _handle);

		ShaderRef& sr = m_shaderRef[_handle.idx];
		sr.m_name.set(_name);

		setName(convert(_handle), _name);
	}

	void Context::setName(TextureHandle _handle, const bx::StringView& _name)
	{
		MYGFX_MUTEX_SCOPE(m_resourceApiLock);
		MYGFX_CHECK_HANDLE("setName", m_textureHandle, _handle);

		TextureRef& ref = m_textureRef[_handle.idx];
		ref.m_name.set(_name);

		setName(convert(_handle), _name);
	}

	void Context::setDirectAccessPtr(TextureHandle _handle, void* _ptr)
	{
		TextureRef& ref = m_textureRef[_handle.idx];
		ref.m_ptr = _ptr;
	}

	void* Context::getDirectAccessPtr(TextureHandle _handle)
	{
		MYGFX_MUTEX_SCOPE(m_resourceApiLock);
		MYGFX_CHECK_HANDLE("getDirectAccessPtr", m_textureHandle, _handle);

		TextureRef& ref = m_textureRef[_handle.idx];
		return ref.m_ptr;
	}

	void Context::destroyTexture(TextureHandle _handle)
	{
		MYGFX_MUTEX_SCOPE(m_resourceApiLock);

		MYGFX_CHECK_HANDLE("destroyTexture", m_textureHandle, _handle);

		if (!isValid(_handle))
		{
			BX_WARN(false, "Passing invalid texture handle to bgfx::destroyTexture");
			return;
		}

		textureDecRef(_handle);
	}

	uint32_t Context::readTexture(TextureHandle _handle, void* _data, uint8_t _mip)
	{
		MYGFX_MUTEX_SCOPE(m_resourceApiLock);

		MYGFX_CHECK_HANDLE("readTexture", m_textureHandle, _handle);

		const TextureRef& ref = m_textureRef[_handle.idx];
		BX_CHECK(_mip < ref.m_numMips, "Invalid mip: %d num mips:", _mip, ref.m_numMips); BX_UNUSED(ref);

		CommandBuffer& cmdbuf = getCommandBuffer(CommandBuffer::ReadTexture);
		cmdbuf.write(_handle);
		cmdbuf.write(_data);
		cmdbuf.write(_mip);
		return m_frames + 2;
	}

	void Context::resizeTexture(TextureHandle _handle, uint16_t _width, uint16_t _height, uint8_t _numMips, uint16_t _numLayers)
	{
		const TextureRef& textureRef = m_textureRef[_handle.idx];
		BX_CHECK(BackbufferRatio::Count != textureRef.m_bbRatio, "");

		getTextureSizeFromRatio(BackbufferRatio::Enum(textureRef.m_bbRatio), _width, _height);
		_numMips = calcNumMips(1 < _numMips, _width, _height);

		BX_TRACE("Resize %3d: %4dx%d %s"
			, _handle.idx
			, _width
			, _height
			, bimg::getName(bimg::TextureFormat::Enum(textureRef.m_format))
		);

		CommandBuffer& cmdbuf = getCommandBuffer(CommandBuffer::ResizeTexture);
		cmdbuf.write(_handle);
		cmdbuf.write(_width);
		cmdbuf.write(_height);
		cmdbuf.write(_numMips);
		cmdbuf.write(_numLayers);
	}

	void Context::textureTakeOwnership(TextureHandle _handle)
	{
		TextureRef& ref = m_textureRef[_handle.idx];
		if (!ref.m_owned)
		{
			ref.m_owned = true;
			textureDecRef(_handle);
		}
	}

	void Context::textureIncRef(TextureHandle _handle)
	{
		TextureRef& ref = m_textureRef[_handle.idx];
		++ref.m_refCount;
	}

	void Context::textureDecRef(TextureHandle _handle)
	{
		TextureRef& ref = m_textureRef[_handle.idx];
		int32_t refs = --ref.m_refCount;
		if (0 == refs)
		{
			ref.m_name.clear();

			if (ref.m_rt)
			{
				m_rtMemoryUsed -= int64_t(ref.m_storageSize);
			}
			else
			{
				m_textureMemoryUsed -= int64_t(ref.m_storageSize);
			}

			bool ok = m_submit->free(_handle); BX_UNUSED(ok);
			BX_CHECK(ok, "Texture handle %d is already destroyed!", _handle.idx);

			CommandBuffer& cmdbuf = getCommandBuffer(CommandBuffer::DestroyTexture);
			cmdbuf.write(_handle);
		}
	}

	void Context::updateTexture(TextureHandle _handle, uint8_t _side, uint8_t _mip, uint16_t _x, uint16_t _y, uint16_t _z, uint16_t _width, uint16_t _height, uint16_t _depth, uint16_t _pitch, const Memory* _mem)
	{
		MYGFX_MUTEX_SCOPE(m_resourceApiLock);

		const TextureRef& textureRef = m_textureRef[_handle.idx];
		if (textureRef.m_immutable)
		{
			BX_WARN(false, "Can't update immutable texture.");
			release(_mem);
			return;
		}

		CommandBuffer& cmdbuf = getCommandBuffer(CommandBuffer::UpdateTexture);
		cmdbuf.write(_handle);
		cmdbuf.write(_side);
		cmdbuf.write(_mip);
		Rect rect;
		rect.m_x = _x;
		rect.m_y = _y;
		rect.m_width = _width;
		rect.m_height = _height;
		cmdbuf.write(rect);
		cmdbuf.write(_z);
		cmdbuf.write(_depth);
		cmdbuf.write(_pitch);
		cmdbuf.write(_mem);
	}

	bool Context::checkFrameBuffer(uint8_t _num, const Attachment* _attachment) const
	{
		uint8_t color = 0;
		uint8_t depth = 0;

		for (uint32_t ii = 0; ii < _num; ++ii)
		{
			TextureHandle texHandle = _attachment[ii].handle;
			if (bimg::isDepth(bimg::TextureFormat::Enum(m_textureRef[texHandle.idx].m_format)))
			{
				++depth;
			}
			else
			{
				++color;
			}
		}

		return color <= g_caps.limits.maxFBAttachments
			&& depth <= 1
			;
	}

	mygfx::FrameBufferHandle Context::createFrameBuffer(uint8_t _num, const Attachment* _attachment, bool _destroyTextures)
	{
		MYGFX_MUTEX_SCOPE(m_resourceApiLock);

		BX_CHECK(checkFrameBuffer(_num, _attachment)
			, "Too many frame buffer attachments (num attachments: %d, max color attachments %d)!"
			, _num
			, g_caps.limits.maxFBAttachments
		);

		FrameBufferHandle handle = { m_frameBufferHandle.alloc() };
		BX_WARN(isValid(handle), "Failed to allocate frame buffer handle.");

		if (isValid(handle))
		{
			CommandBuffer& cmdbuf = getCommandBuffer(CommandBuffer::CreateFrameBuffer);
			cmdbuf.write(handle);
			cmdbuf.write(false);
			cmdbuf.write(_num);

			FrameBufferRef& ref = m_frameBufferRef[handle.idx];
			ref.m_window = false;
			bx::memSet(ref.un.m_th, 0xff, sizeof(ref.un.m_th));
			BackbufferRatio::Enum bbRatio = BackbufferRatio::Enum(m_textureRef[_attachment[0].handle.idx].m_bbRatio);
			for (uint32_t ii = 0; ii < _num; ++ii)
			{
				TextureHandle texHandle = _attachment[ii].handle;
				MYGFX_CHECK_HANDLE("createFrameBuffer texture handle", m_textureHandle, texHandle);
				BX_CHECK(bbRatio == m_textureRef[texHandle.idx].m_bbRatio, "Mismatch in texture back-buffer ratio.");
				BX_UNUSED(bbRatio);

				ref.un.m_th[ii] = texHandle;
				textureIncRef(texHandle);
			}

			cmdbuf.write(_attachment, sizeof(Attachment) * _num);
		}

		if (_destroyTextures)
		{
			for (uint32_t ii = 0; ii < _num; ++ii)
			{
				textureTakeOwnership(_attachment[ii].handle);
			}
		}

		return handle;
	}

	mygfx::FrameBufferHandle Context::createFrameBuffer(void* _nwh, uint16_t _width, uint16_t _height, TextureFormat::Enum _format, TextureFormat::Enum _depthFormat)
	{
		MYGFX_MUTEX_SCOPE(m_resourceApiLock);

		FrameBufferHandle handle = { m_frameBufferHandle.alloc() };
		BX_WARN(isValid(handle), "Failed to allocate frame buffer handle.");

		if (isValid(handle))
		{
			CommandBuffer& cmdbuf = getCommandBuffer(CommandBuffer::CreateFrameBuffer);
			cmdbuf.write(handle);
			cmdbuf.write(true);
			cmdbuf.write(_nwh);
			cmdbuf.write(_width);
			cmdbuf.write(_height);
			cmdbuf.write(_format);
			cmdbuf.write(_depthFormat);

			FrameBufferRef& ref = m_frameBufferRef[handle.idx];
			ref.m_window = true;
			ref.un.m_nwh = _nwh;
		}

		return handle;
	}

	mygfx::TextureHandle Context::getTexture(FrameBufferHandle _handle, uint8_t _attachment)
	{
		MYGFX_MUTEX_SCOPE(m_resourceApiLock);

		MYGFX_CHECK_HANDLE("getTexture", m_frameBufferHandle, _handle);

		const FrameBufferRef& ref = m_frameBufferRef[_handle.idx];
		if (!ref.m_window)
		{
			const uint32_t attachment = bx::min<uint32_t>(_attachment, MYGFX_CONFIG_MAX_FRAME_BUFFER_ATTACHMENTS);
			return ref.un.m_th[attachment];
		}

		return MYGFX_INVALID_HANDLE;
	}

	void Context::destroyFrameBuffer(FrameBufferHandle _handle)
	{
		MYGFX_MUTEX_SCOPE(m_resourceApiLock);

		MYGFX_CHECK_HANDLE("destroyFrameBuffer", m_frameBufferHandle, _handle);
		bool ok = m_submit->free(_handle); BX_UNUSED(ok);
		BX_CHECK(ok, "Frame buffer handle %d is already destroyed!", _handle.idx);

		CommandBuffer& cmdbuf = getCommandBuffer(CommandBuffer::DestroyFrameBuffer);
		cmdbuf.write(_handle);

		FrameBufferRef& ref = m_frameBufferRef[_handle.idx];
		if (!ref.m_window)
		{
			for (uint32_t ii = 0; ii < BX_COUNTOF(ref.un.m_th); ++ii)
			{
				TextureHandle th = ref.un.m_th[ii];
				if (isValid(th))
				{
					textureDecRef(th);
				}
			}
		}
	}

	void Context::getUniformInfo(UniformHandle _handle, UniformInfo& _info)
	{
		MYGFX_MUTEX_SCOPE(m_resourceApiLock);

		MYGFX_CHECK_HANDLE("getUniformInfo", m_uniformHandle, _handle);

		UniformRef& uniform = m_uniformRef[_handle.idx];
		bx::strCopy(_info.name, sizeof(_info.name), uniform.m_name.getPtr());
		_info.type = uniform.m_type;
		_info.num = uniform.m_num;
	}

	void Context::destroyUniform(UniformHandle _handle)
	{
		MYGFX_MUTEX_SCOPE(m_resourceApiLock);

		MYGFX_CHECK_HANDLE("destroyUniform", m_uniformHandle, _handle);

		UniformRef& uniform = m_uniformRef[_handle.idx];
		BX_CHECK(uniform.m_refCount > 0, "Destroying already destroyed uniform %d.", _handle.idx);
		int32_t refs = --uniform.m_refCount;

		if (0 == refs)
		{
			bool ok = m_submit->free(_handle); BX_UNUSED(ok);
			BX_CHECK(ok, "Uniform handle %d is already destroyed!", _handle.idx);

			uniform.m_name.clear();
			m_uniformHashMap.removeByHandle(_handle.idx);

			CommandBuffer& cmdbuf = getCommandBuffer(CommandBuffer::DestroyUniform);
			cmdbuf.write(_handle);
		}
	}

	mygfx::OcclusionQueryHandle Context::createOcclusionQuery()
	{
		MYGFX_MUTEX_SCOPE(m_resourceApiLock);

		OcclusionQueryHandle handle = { m_occlusionQueryHandle.alloc() };
		if (isValid(handle))
		{
			m_submit->m_occlusion[handle.idx] = INT32_MIN;

			CommandBuffer& cmdbuf = getCommandBuffer(CommandBuffer::InvalidateOcclusionQuery);
			cmdbuf.write(handle);
		}

		return handle;
	}

	mygfx::OcclusionQueryResult::Enum Context::getResult(OcclusionQueryHandle _handle, int32_t* _result)
	{
		MYGFX_MUTEX_SCOPE(m_resourceApiLock);

		MYGFX_CHECK_HANDLE("getResult", m_occlusionQueryHandle, _handle);

		switch (m_submit->m_occlusion[_handle.idx])
		{
		case 0:         return OcclusionQueryResult::Invisible;
		case INT32_MIN: return OcclusionQueryResult::NoResult;
		default: break;
		}

		if (NULL != _result)
		{
			*_result = m_submit->m_occlusion[_handle.idx];
		}

		return OcclusionQueryResult::Visible;
	}

	void Context::destroyOcclusionQuery(OcclusionQueryHandle _handle)
	{
		MYGFX_MUTEX_SCOPE(m_resourceApiLock);

		MYGFX_CHECK_HANDLE("destroyOcclusionQuery", m_occlusionQueryHandle, _handle);

		m_freeOcclusionQueryHandle[m_numFreeOcclusionQueryHandles++] = _handle;
	}

	void Context::requestScreenShot(FrameBufferHandle _handle, const char* _filePath)
	{
		MYGFX_MUTEX_SCOPE(m_resourceApiLock);

		MYGFX_CHECK_HANDLE_INVALID_OK("requestScreenShot", m_frameBufferHandle, _handle);

		if (isValid(_handle))
		{
			FrameBufferRef& ref = m_frameBufferRef[_handle.idx];
			if (!ref.m_window)
			{
				BX_TRACE("requestScreenShot can be done only for window frame buffer handles (handle: %d).", _handle.idx);
				return;
			}
		}

		CommandBuffer& cmdbuf = getCommandBuffer(CommandBuffer::RequestScreenShot);
		uint16_t len = (uint16_t)bx::strLen(_filePath) + 1;
		cmdbuf.write(_handle);
		cmdbuf.write(len);
		cmdbuf.write(_filePath, len);
	}

	void Context::setPaletteColor(uint8_t _index, const float _rgba[4])
	{
		MYGFX_MUTEX_SCOPE(m_resourceApiLock);

		BX_CHECK(_index < MYGFX_CONFIG_MAX_COLOR_PALETTE, "Color palette index out of bounds %d (max: %d)."
			, _index
			, MYGFX_CONFIG_MAX_COLOR_PALETTE
		);
		bx::memCopy(&m_clearColor[_index][0], _rgba, 16);
		m_colorPaletteDirty = 2;
	}

	void Context::setViewName(ViewId _id, const char* _name)
	{
		MYGFX_MUTEX_SCOPE(m_resourceApiLock);

		CommandBuffer& cmdbuf = getCommandBuffer(CommandBuffer::UpdateViewName);
		cmdbuf.write(_id);
		uint16_t len = (uint16_t)bx::strLen(_name) + 1;
		cmdbuf.write(len);
		cmdbuf.write(_name, len);
	}

	void Context::setViewOrder(ViewId _id, uint16_t _num, const ViewId* _order)
	{
		const uint32_t num = bx::min(_id + _num, MYGFX_CONFIG_MAX_VIEWS) - _id;
		if (NULL == _order)
		{
			for (uint32_t ii = 0; ii < num; ++ii)
			{
				ViewId id = ViewId(ii + _id);
				m_viewRemap[id] = id;
			}
		}
		else
		{
			bx::memCopy(&m_viewRemap[_id], _order, num * sizeof(ViewId));
		}
	}

	void Context::destroyShader(ShaderHandle _handle)
	{
		MYGFX_MUTEX_SCOPE(m_resourceApiLock);

		MYGFX_CHECK_HANDLE("destroyShader", m_shaderHandle, _handle);

		if (!isValid(_handle))
		{
			BX_WARN(false, "Passing invalid shader handle to bgfx::destroyShader.");
			return;
		}

		shaderDecRef(_handle);
	}

	void Context::shaderTakeOwnership(ShaderHandle _handle)
	{
		shaderDecRef(_handle);
	}

	void Context::shaderIncRef(ShaderHandle _handle)
	{
		ShaderRef& sr = m_shaderRef[_handle.idx];
		++sr.m_refCount;
	}

	void Context::shaderDecRef(ShaderHandle _handle)
	{
		ShaderRef& sr = m_shaderRef[_handle.idx];
		int32_t refs = --sr.m_refCount;
		if (0 == refs)
		{
			bool ok = m_submit->free(_handle); BX_UNUSED(ok);
			BX_CHECK(ok, "Shader handle %d is already destroyed!", _handle.idx);

			CommandBuffer& cmdbuf = getCommandBuffer(CommandBuffer::DestroyShader);
			cmdbuf.write(_handle);

			if (0 != sr.m_num)
			{
				for (uint32_t ii = 0, num = sr.m_num; ii < num; ++ii)
				{
					destroyUniform(sr.m_uniforms[ii]);
				}

				BX_FREE(g_allocator, sr.m_uniforms);
				sr.m_uniforms = NULL;
				sr.m_num = 0;
			}

			m_shaderHashMap.removeByHandle(_handle.idx);
		}
	}

	mygfx::ProgramHandle Context::createProgram(ShaderHandle _vsh, ShaderHandle _fsh, bool _destroyShaders)
	{
		MYGFX_MUTEX_SCOPE(m_resourceApiLock);

		if (!isValid(_vsh)
			|| !isValid(_fsh))
		{
			BX_TRACE("Vertex/fragment shader is invalid (vsh %d, fsh %d).", _vsh.idx, _fsh.idx);
			return MYGFX_INVALID_HANDLE;
		}

		ProgramHandle handle = { m_programHashMap.find(uint32_t(_fsh.idx << 16) | _vsh.idx) };
		if (isValid(handle))
		{
			ProgramRef& pr = m_programRef[handle.idx];
			++pr.m_refCount;
			shaderIncRef(pr.m_vsh);
			shaderIncRef(pr.m_fsh);
		}
		else
		{
			const ShaderRef& vsr = m_shaderRef[_vsh.idx];
			const ShaderRef& fsr = m_shaderRef[_fsh.idx];
			if (vsr.m_hashOut != fsr.m_hashIn)
			{
				BX_TRACE("Vertex shader output doesn't match fragment shader input.");
				return MYGFX_INVALID_HANDLE;
			}

			handle.idx = m_programHandle.alloc();

			BX_WARN(isValid(handle), "Failed to allocate program handle.");
			if (isValid(handle))
			{
				shaderIncRef(_vsh);
				shaderIncRef(_fsh);
				ProgramRef& pr = m_programRef[handle.idx];
				pr.m_vsh = _vsh;
				pr.m_fsh = _fsh;
				pr.m_refCount = 1;

				const uint32_t key = uint32_t(_fsh.idx << 16) | _vsh.idx;
				bool ok = m_programHashMap.insert(key, handle.idx);
				BX_CHECK(ok, "Program already exists (key: %x, handle: %3d)!", key, handle.idx); BX_UNUSED(ok);

				CommandBuffer& cmdbuf = getCommandBuffer(CommandBuffer::CreateProgram);
				cmdbuf.write(handle);
				cmdbuf.write(_vsh);
				cmdbuf.write(_fsh);
			}
		}

		if (_destroyShaders)
		{
			shaderTakeOwnership(_vsh);
			shaderTakeOwnership(_fsh);
		}

		return handle;
	}

	mygfx::ProgramHandle Context::createProgram(ShaderHandle _vsh, bool _destroyShader)
	{
		MYGFX_MUTEX_SCOPE(m_resourceApiLock);

		if (!isValid(_vsh))
		{
			BX_WARN(false, "Compute shader is invalid (vsh %d).", _vsh.idx);
			return MYGFX_INVALID_HANDLE;
		}

		ProgramHandle handle = { m_programHashMap.find(_vsh.idx) };

		if (isValid(handle))
		{
			ProgramRef& pr = m_programRef[handle.idx];
			++pr.m_refCount;
			shaderIncRef(pr.m_vsh);
		}
		else
		{
			handle.idx = m_programHandle.alloc();

			BX_WARN(isValid(handle), "Failed to allocate program handle.");
			if (isValid(handle))
			{
				shaderIncRef(_vsh);
				ProgramRef& pr = m_programRef[handle.idx];
				pr.m_vsh = _vsh;
				ShaderHandle fsh = MYGFX_INVALID_HANDLE;
				pr.m_fsh = fsh;
				pr.m_refCount = 1;

				const uint32_t key = uint32_t(_vsh.idx);
				bool ok = m_programHashMap.insert(key, handle.idx);
				BX_CHECK(ok, "Program already exists (key: %x, handle: %3d)!", key, handle.idx); BX_UNUSED(ok);

				CommandBuffer& cmdbuf = getCommandBuffer(CommandBuffer::CreateProgram);
				cmdbuf.write(handle);
				cmdbuf.write(_vsh);
				cmdbuf.write(fsh);
			}
		}

		if (_destroyShader)
		{
			shaderTakeOwnership(_vsh);
		}

		return handle;
	}

	void Context::destroyProgram(ProgramHandle _handle)
	{
		MYGFX_MUTEX_SCOPE(m_resourceApiLock);

		MYGFX_CHECK_HANDLE("destroyProgram", m_programHandle, _handle);

		ProgramRef& pr = m_programRef[_handle.idx];
		shaderDecRef(pr.m_vsh);

		if (isValid(pr.m_fsh))
		{
			shaderDecRef(pr.m_fsh);
		}

		int32_t refs = --pr.m_refCount;
		if (0 == refs)
		{
			bool ok = m_submit->free(_handle); BX_UNUSED(ok);
			BX_CHECK(ok, "Program handle %d is already destroyed!", _handle.idx);

			CommandBuffer& cmdbuf = getCommandBuffer(CommandBuffer::DestroyProgram);
			cmdbuf.write(_handle);

			m_programHashMap.removeByHandle(_handle.idx);
		}
	}

	mygfx::TextureHandle Context::createTexture(const Memory* _mem, uint64_t _flags, uint8_t _skip, TextureInfo* _info, BackbufferRatio::Enum _ratio, bool _immutable)
	{
		MYGFX_MUTEX_SCOPE(m_resourceApiLock);

		TextureInfo ti;
		if (NULL == _info)
		{
			_info = &ti;
		}

		bimg::ImageContainer imageContainer;
		if (bimg::imageParse(imageContainer, _mem->data, _mem->size))
		{
			calcTextureSize(*_info
				, (uint16_t)imageContainer.m_width
				, (uint16_t)imageContainer.m_height
				, (uint16_t)imageContainer.m_depth
				, imageContainer.m_cubeMap
				, imageContainer.m_numMips > 1
				, imageContainer.m_numLayers
				, TextureFormat::Enum(imageContainer.m_format)
			);
		}
		else
		{
			_info->format = TextureFormat::Unknown;
			_info->storageSize = 0;
			_info->width = 0;
			_info->height = 0;
			_info->depth = 0;
			_info->numMips = 0;
			_info->bitsPerPixel = 0;
			_info->cubeMap = false;

			return MYGFX_INVALID_HANDLE;
		}

		TextureHandle handle = { m_textureHandle.alloc() };
		BX_WARN(isValid(handle), "Failed to allocate texture handle.");

		if (!isValid(handle))
		{
			release(_mem);
			return MYGFX_INVALID_HANDLE;
		}

		TextureRef& ref = m_textureRef[handle.idx];
		ref.init(
			_ratio
			, _info->format
			, _info->storageSize
			, imageContainer.m_numMips
			, imageContainer.m_numLayers
			, 0 != (g_caps.supported & MYGFX_CAPS_TEXTURE_DIRECT_ACCESS)
			, _immutable
			, 0 != (_flags & MYGFX_TEXTURE_RT_MASK)
		);

		if (ref.m_rt)
		{
			m_rtMemoryUsed += int64_t(ref.m_storageSize);
		}
		else
		{
			m_textureMemoryUsed += int64_t(ref.m_storageSize);
		}

		CommandBuffer& cmdbuf = getCommandBuffer(CommandBuffer::CreateTexture);
		cmdbuf.write(handle);
		cmdbuf.write(_mem);
		cmdbuf.write(_flags);
		cmdbuf.write(_skip);

		return handle;
	}

	void Context::destroyDynamicIndexBuffer(DynamicIndexBufferHandle _handle)
	{
		MYGFX_MUTEX_SCOPE(m_resourceApiLock);

		MYGFX_CHECK_HANDLE("destroyDynamicIndexBuffer", m_dynamicIndexBufferHandle, _handle);

		m_freeDynamicIndexBufferHandle[m_numFreeDynamicIndexBufferHandles++] = _handle;
	}

	void Context::destroyDynamicIndexBufferInternal(DynamicIndexBufferHandle _handle)
	{
		DynamicIndexBuffer& dib = m_dynamicIndexBuffers[_handle.idx];

		if (0 != (dib.m_flags & MYGFX_BUFFER_COMPUTE_READ_WRITE))
		{
			destroyIndexBuffer(dib.m_handle);
		}
		else
		{
			m_dynIndexBufferAllocator.free(uint64_t(dib.m_handle.idx) << 32 | dib.m_offset);
			if (m_dynIndexBufferAllocator.compact())
			{
				for (uint64_t ptr = m_dynIndexBufferAllocator.remove(); 0 != ptr; ptr = m_dynIndexBufferAllocator.remove())
				{
					IndexBufferHandle handle = { uint16_t(ptr >> 32) };
					destroyIndexBuffer(handle);
				}
			}
		}

		m_dynamicIndexBufferHandle.free(_handle.idx);
	}

	uint64_t Context::allocDynamicVertexBuffer(uint32_t _size, uint16_t _flags)
	{
		uint64_t ptr = m_dynVertexBufferAllocator.alloc(_size);
		if (ptr == NonLocalAllocator::kInvalidBlock)
		{
			VertexBufferHandle vertexBufferHandle = { m_vertexBufferHandle.alloc() };
			BX_WARN(isValid(vertexBufferHandle), "Failed to allocate dynamic vertex buffer handle.");
			if (!isValid(vertexBufferHandle))
			{
				return NonLocalAllocator::kInvalidBlock;
			}

			const uint32_t allocSize = bx::max<uint32_t>(MYGFX_CONFIG_DYNAMIC_VERTEX_BUFFER_SIZE, _size);

			VertexBuffer& vb = m_vertexBuffers[vertexBufferHandle.idx];
			vb.m_size = allocSize;
			vb.m_stride = 0;

			CommandBuffer& cmdbuf = getCommandBuffer(CommandBuffer::CreateDynamicVertexBuffer);
			cmdbuf.write(vertexBufferHandle);
			cmdbuf.write(allocSize);
			cmdbuf.write(_flags);

			m_dynVertexBufferAllocator.add(uint64_t(vertexBufferHandle.idx) << 32, allocSize);
			ptr = m_dynVertexBufferAllocator.alloc(_size);
		}

		return ptr;
	}

}