#pragma once

#include "mygfx/mygfx.h"
#include "mygfx/platform.h"
#include <bx/allocator.h>

namespace mygfx
{
	//
	struct Context;

	extern InternalData g_internalData;
	extern PlatformData g_platformData;
	extern bool g_platformDataChangedSinceReset;

	extern const uint32_t g_uniformTypeSize[UniformType::Count + 1];
	extern CallbackI* g_callback;
	extern bx::AllocatorI* g_allocator;
	extern Caps g_caps;

	static Context* s_ctx = NULL;

#if MYGFX_CONFIG_MULTITHREADED && !defined(BX_THREAD_LOCAL)
	class ThreadData
	{
		BX_CLASS(ThreadData
			, NO_COPY
			, NO_ASSIGNMENT
		);

	public:
		ThreadData(uintptr_t _rhs)
		{
			union { uintptr_t ui; void* ptr; } cast = { _rhs };
			m_tls.set(cast.ptr);
		}

		operator uintptr_t() const
		{
			union { uintptr_t ui; void* ptr; } cast;
			cast.ptr = m_tls.get();
			return cast.ui;
		}

		uintptr_t operator=(uintptr_t _rhs)
		{
			union { uintptr_t ui; void* ptr; } cast = { _rhs };
			m_tls.set(cast.ptr);
			return _rhs;
		}

		bool operator==(uintptr_t _rhs) const
		{
			uintptr_t lhs = *this;
			return lhs == _rhs;
		}

	private:
		bx::TlsData m_tls;
	};

	static ThreadData s_threadIndex(0);
#elif !MYGFX_CONFIG_MULTITHREADED
	static uint32_t s_threadIndex(0);
#else
	static BX_THREAD_LOCAL uint32_t s_threadIndex(0);
#endif
}
