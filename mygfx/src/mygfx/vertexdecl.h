
#ifndef MYGFX_VERTEXDECL_H_HEADER_GUARD
#define MYGFX_VERTEXDECL_H_HEADER_GUARD

#include "mygfx/mygfx.h"
#include <bx/readerwriter.h>

namespace mygfx
{
	struct VertexDeclRef
	{
		VertexDeclRef()
		{
		}

		void init()
		{
			bx::memSet(m_vertexDeclRef, 0, sizeof(m_vertexDeclRef));
			bx::memSet(m_vertexBufferRef, 0xff, sizeof(m_vertexBufferRef));
			bx::memSet(m_dynamicVertexBufferRef, 0xff, sizeof(m_dynamicVertexBufferRef));
		}

		template <uint16_t MaxHandlesT>
		void shutdown(bx::HandleAllocT<MaxHandlesT>& _handleAlloc)
		{
			for (uint16_t ii = 0, num = _handleAlloc.getNumHandles(); ii < num; ++ii)
			{
				VertexDeclHandle handle = { _handleAlloc.getHandleAt(ii) };
				m_vertexDeclRef[handle.idx] = 0;
				m_vertexDeclMap.removeByHandle(handle.idx);
				_handleAlloc.free(handle.idx);
			}

			m_vertexDeclMap.reset();
		}

		VertexDeclHandle find(uint32_t _hash)
		{
			VertexDeclHandle handle = { m_vertexDeclMap.find(_hash) };
			return handle;
		}

		void add(VertexDeclHandle _declHandle, uint32_t _hash)
		{
			m_vertexDeclRef[_declHandle.idx]++;
			m_vertexDeclMap.insert(_hash, _declHandle.idx);
		}

		void add(VertexBufferHandle _handle, VertexDeclHandle _declHandle, uint32_t _hash)
		{
			BX_CHECK(m_vertexBufferRef[_handle.idx].idx == kInvalidHandle, "");
			m_vertexBufferRef[_handle.idx] = _declHandle;
			m_vertexDeclRef[_declHandle.idx]++;
			m_vertexDeclMap.insert(_hash, _declHandle.idx);
		}

		void add(DynamicVertexBufferHandle _handle, VertexDeclHandle _declHandle, uint32_t _hash)
		{
			BX_CHECK(m_dynamicVertexBufferRef[_handle.idx].idx == kInvalidHandle, "");
			m_dynamicVertexBufferRef[_handle.idx] = _declHandle;
			m_vertexDeclRef[_declHandle.idx]++;
			m_vertexDeclMap.insert(_hash, _declHandle.idx);
		}

		VertexDeclHandle release(VertexDeclHandle _declHandle)
		{
			if (isValid(_declHandle))
			{
				m_vertexDeclRef[_declHandle.idx]--;

				if (0 == m_vertexDeclRef[_declHandle.idx])
				{
					m_vertexDeclMap.removeByHandle(_declHandle.idx);
					return _declHandle;
				}
			}

			return MYGFX_INVALID_HANDLE;
		}

		VertexDeclHandle release(VertexBufferHandle _handle)
		{
			VertexDeclHandle declHandle = m_vertexBufferRef[_handle.idx];
			declHandle = release(declHandle);
			m_vertexBufferRef[_handle.idx].idx = kInvalidHandle;

			return declHandle;
		}

		VertexDeclHandle release(DynamicVertexBufferHandle _handle)
		{
			VertexDeclHandle declHandle = m_dynamicVertexBufferRef[_handle.idx];
			declHandle = release(declHandle);
			m_dynamicVertexBufferRef[_handle.idx].idx = kInvalidHandle;

			return declHandle;
		}

		typedef bx::HandleHashMapT<MYGFX_CONFIG_MAX_VERTEX_DECLS * 2> VertexDeclMap;
		VertexDeclMap m_vertexDeclMap;

		uint16_t m_vertexDeclRef[MYGFX_CONFIG_MAX_VERTEX_DECLS];
		VertexDeclHandle m_vertexBufferRef[MYGFX_CONFIG_MAX_VERTEX_BUFFERS];
		VertexDeclHandle m_dynamicVertexBufferRef[MYGFX_CONFIG_MAX_DYNAMIC_VERTEX_BUFFERS];
	};

	///
	void initAttribTypeSizeTable(RendererType::Enum _type);

	/// Returns attribute name.
	const char* getAttribName(Attrib::Enum _attr);

	///
	const char* getAttribNameShort(Attrib::Enum _attr);

	/// Dump vertex declaration into debug output.
	void dump(const VertexDecl& _decl);

	///
	Attrib::Enum idToAttrib(uint16_t id);

	///
	uint16_t attribToId(Attrib::Enum _attr);

	///
	AttribType::Enum idToAttribType(uint16_t id);

	///
	int32_t write(bx::WriterI* _writer, const VertexDecl& _decl, bx::Error* _err = NULL);

	///
	int32_t read(bx::ReaderI* _reader, VertexDecl& _decl, bx::Error* _err = NULL);

} // namespace bgfx

#endif