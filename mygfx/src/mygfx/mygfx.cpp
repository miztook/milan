#include <bx/platform.h>

#include "mygfx_p.h"
#include "Globals.h"

#include <bx/file.h>
#include <bx/mutex.h>

//mygfx.h
namespace mygfx
{
	PlatformData::PlatformData()
		: ndt(NULL)
		, nwh(NULL)
		, context(NULL)
		, backBuffer(NULL)
		, backBufferDS(NULL)
	{
	}

	Resolution::Resolution()
		: format(TextureFormat::RGBA8)
		, width(1280)
		, height(720)
		, reset(MYGFX_RESET_NONE)
		, numBackBuffers(2)
		, maxFrameLatency(0)
	{
	}

	Init::Init()
		: type(RendererType::Count)
		, vendorId(MYGFX_PCI_ID_NONE)
		, deviceId(0)
		, debug(BX_ENABLED(MYGFX_CONFIG_DEBUG))
		, profile(BX_ENABLED(MYGFX_CONFIG_DEBUG_PIX))
		, callback(NULL)
	{
		limits.maxEncoders = MYGFX_CONFIG_DEFAULT_MAX_ENCODERS;
		limits.transientVbSize = MYGFX_CONFIG_TRANSIENT_VERTEX_BUFFER_SIZE;
		limits.transientIbSize = MYGFX_CONFIG_TRANSIENT_INDEX_BUFFER_SIZE;
	}

	//
	struct CallbackStub : public CallbackI
	{
		virtual ~CallbackStub()
		{
		}

		virtual void fatal(const char* _filePath, uint16_t _line, Fatal::Enum _code, const char* _str) override
		{
			if (Fatal::DebugCheck == _code)
			{
				bx::debugBreak();
			}
			else
			{
				mygfx::trace(_filePath, _line, "MYGFX 0x%08x: %s\n", _code, _str);
				BX_UNUSED(_code, _str);
				abort();
			}
		}

		virtual void traceVargs(const char* _filePath, uint16_t _line, const char* _format, va_list _argList) override
		{
			char temp[2048];
			char* out = temp;
			va_list argListCopy;
			va_copy(argListCopy, _argList);
			int32_t len = bx::snprintf(out, sizeof(temp), "%s (%d): ", _filePath, _line);
			int32_t total = len + bx::vsnprintf(out + len, sizeof(temp) - len, _format, argListCopy);
			va_end(argListCopy);
			if ((int32_t)sizeof(temp) < total)
			{
				out = (char*)alloca(total + 1);
				bx::memCopy(out, temp, len);
				bx::vsnprintf(out + len, total - len, _format, _argList);
			}
			out[total] = '\0';
			bx::debugOutput(out);
		}
	};

	const size_t kNaturalAlignment = 8;

	class AllocatorStub : public bx::AllocatorI
	{
	public:
		AllocatorStub()
#if MYGFX_CONFIG_MEMORY_TRACKING
			: m_numBlocks(0)
			, m_maxBlocks(0)
#endif // BGFX_CONFIG_MEMORY_TRACKING
		{
		}

		virtual void* realloc(void* _ptr, size_t _size, size_t _align, const char* _file, uint32_t _line) override
		{
			if (0 == _size)
			{
				if (NULL != _ptr)
				{
					if (kNaturalAlignment >= _align)
					{
#if MYGFX_CONFIG_MEMORY_TRACKING
						{
							bx::MutexScope scope(m_mutex);
							BX_CHECK(m_numBlocks > 0, "Number of blocks is 0. Possible alloc/free mismatch?");
							--m_numBlocks;
						}
#endif // BGFX_CONFIG_MEMORY_TRACKING

						::free(_ptr);
					}
					else
					{
						bx::alignedFree(this, _ptr, _align, _file, _line);
					}
				}

				return NULL;
			}
			else if (NULL == _ptr)
			{
				if (kNaturalAlignment >= _align)
				{
#if MYGFX_CONFIG_MEMORY_TRACKING
					{
						bx::MutexScope scope(m_mutex);
						++m_numBlocks;
						m_maxBlocks = bx::max(m_maxBlocks, m_numBlocks);
					}
#endif // BGFX_CONFIG_MEMORY_TRACKING

					return ::malloc(_size);
				}

				return bx::alignedAlloc(this, _size, _align, _file, _line);
			}

			if (kNaturalAlignment >= _align)
			{
#if MYGFX_CONFIG_MEMORY_TRACKING
				if (NULL == _ptr)
				{
					bx::MutexScope scope(m_mutex);
					++m_numBlocks;
					m_maxBlocks = bx::max(m_maxBlocks, m_numBlocks);
				}
#endif // BGFX_CONFIG_MEMORY_TRACKING

				return ::realloc(_ptr, _size);
			}

			return bx::alignedRealloc(this, _ptr, _size, _align, _file, _line);
		}

		void checkLeaks();

	protected:
#if MYGFX_CONFIG_MEMORY_TRACKING
		bx::Mutex m_mutex;
		uint32_t m_numBlocks;
		uint32_t m_maxBlocks;
#endif // BGFX_CONFIG_MEMORY_TRACKING
	};

	static CallbackStub*  s_callbackStub = NULL;
	static AllocatorStub* s_allocatorStub = NULL;


	void reset(uint32_t _width, uint32_t _height, uint32_t _flags, TextureFormat::Enum _format)
	{
		MYGFX_CHECK_API_THREAD();
		BX_CHECK(0 == (_flags&MYGFX_RESET_RESERVED_MASK), "Do not set reset reserved flags!");
		s_ctx->reset(_width, _height, _flags, _format);
	}

	Encoder* begin(bool _forThread)
	{
		return s_ctx->begin(_forThread);
	}

	void end(Encoder* _encoder)
	{
		s_ctx->end(_encoder);
	}

	uint32_t frame(bool _capture)
	{
		MYGFX_CHECK_API_THREAD();
		return s_ctx->frame(_capture);
	}
}

