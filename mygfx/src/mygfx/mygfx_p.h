#ifndef MYGFX_P_H_HEADER_GUARD
#define MYGFX_P_H_HEADER_GUARD

#include <bx/platform.h>

#ifndef MYGFX_CONFIG_DEBUG
#	define MYGFX_CONFIG_DEBUG 0
#endif // MYGFX_CONFIG_DEBUG

#if MYGFX_CONFIG_DEBUG || BX_COMPILER_CLANG_ANALYZER
#	define BX_TRACE _BX_TRACE
#	define BX_WARN  _BX_WARN
#	define BX_CHECK _BX_CHECK
#	define BX_CONFIG_ALLOCATOR_DEBUG 1
#endif // MYGFX_CONFIG_DEBUG

#include "mygfx/mygfx.h"
#include "config.h"

#include <inttypes.h>

// Check handle, cannot be bgfx::kInvalidHandle and must be valid.
#define MYGFX_CHECK_HANDLE(_desc, _handleAlloc, _handle)    \
			BX_CHECK(isValid(_handle)                      \
				&& _handleAlloc.isValid(_handle.idx)       \
				, "Invalid handle. %s handle: %d (max %d)" \
				, _desc                                    \
				, _handle.idx                              \
				, _handleAlloc.getMaxHandles()             \
				)

// Check handle, it's ok to be bgfx::kInvalidHandle or must be valid.
#define MYGFX_CHECK_HANDLE_INVALID_OK(_desc, _handleAlloc, _handle) \
			BX_CHECK(!isValid(_handle)                             \
				|| _handleAlloc.isValid(_handle.idx)               \
				, "Invalid handle. %s handle: %d (max %d)"         \
				, _desc                                            \
				, _handle.idx                                      \
				, _handleAlloc.getMaxHandles()                     \
				)

#if MYGFX_CONFIG_MULTITHREADED
#	define MYGFX_MUTEX_SCOPE(_mutex) bx::MutexScope BX_CONCATENATE(mutexScope, __LINE__)(_mutex)
#else
#	define MYGFX_MUTEX_SCOPE(_mutex) BX_NOOP()
#endif // MYGFX_CONFIG_MULTITHREADED

#if MYGFX_CONFIG_PROFILER
#	define MYGFX_PROFILER_SCOPE(_name, _abgr) ProfilerScope BX_CONCATENATE(profilerScope, __LINE__)(_name, _abgr, __FILE__, uint16_t(__LINE__) )
#	define MYGFX_PROFILER_BEGIN(_name, _abgr) g_callback->profilerBeginLiteral(_name, _abgr, __FILE__, uint16_t(__LINE__) )
#	define MYGFX_PROFILER_END() g_callback->profilerEnd()
#	define MYGFX_PROFILER_SET_CURRENT_THREAD_NAME(_name) BX_NOOP()
#else
#	define MYGFX_PROFILER_SCOPE(_name, _abgr) BX_NOOP()
#	define MYGFX_PROFILER_BEGIN(_name, _abgr) BX_NOOP()
#	define MYGFX_PROFILER_END() BX_NOOP()
#	define MYGFX_PROFILER_SET_CURRENT_THREAD_NAME(_name) BX_NOOP()
#endif // MYGFX_PROFILER_SCOPE

namespace mygfx
{
#if BX_COMPILER_CLANG_ANALYZER
	void __attribute__((analyzer_noreturn)) fatal(Fatal::Enum _code, const char* _format, ...);
#else
	void fatal(const char* _filePath, uint16_t _line, Fatal::Enum _code, const char* _format, ...);
#endif // BX_COMPILER_CLANG_ANALYZER

	void trace(const char* _filePath, uint16_t _line, const char* _format, ...);

	inline bool operator==(const VertexDeclHandle& _lhs, const VertexDeclHandle& _rhs) { return _lhs.idx == _rhs.idx; }
	inline bool operator==(const UniformHandle& _lhs, const UniformHandle&    _rhs) { return _lhs.idx == _rhs.idx; }
}

#define _BX_TRACE(_format, ...)                                                                     \
				BX_MACRO_BLOCK_BEGIN                                                                \
					mygfx::trace(__FILE__, uint16_t(__LINE__), "BGFX " _format "\n", ##__VA_ARGS__); \
				BX_MACRO_BLOCK_END

#define _BX_WARN(_condition, _format, ...)                        \
				BX_MACRO_BLOCK_BEGIN                              \
					if (!BX_IGNORE_C4127(_condition) )            \
					{                                             \
						BX_TRACE("WARN " _format, ##__VA_ARGS__); \
					}                                             \
				BX_MACRO_BLOCK_END

#define _BX_CHECK(_condition, _format, ...)                                                                         \
				BX_MACRO_BLOCK_BEGIN                                                                                \
					if (!BX_IGNORE_C4127(_condition) )                                                              \
					{                                                                                               \
						BX_TRACE("CHECK " _format, ##__VA_ARGS__);                                                  \
						mygfx::fatal(__FILE__, uint16_t(__LINE__), mygfx::Fatal::DebugCheck, _format, ##__VA_ARGS__); \
					}                                                                                               \
				BX_MACRO_BLOCK_END

#define MYGFX_FATAL(_condition, _err, _format, ...)                                     \
			BX_MACRO_BLOCK_BEGIN                                                       \
				if (!BX_IGNORE_C4127(_condition) )                                     \
				{                                                                      \
					fatal(__FILE__, uint16_t(__LINE__), _err, _format, ##__VA_ARGS__); \
				}                                                                      \
			BX_MACRO_BLOCK_END


#include <bx/allocator.h>
#include <bx/bx.h>
#include <bx/cpu.h>
#include <bx/debug.h>
#include <bx/endian.h>
#include <bx/float4x4_t.h>
#include <bx/handlealloc.h>
#include <bx/hash.h>
#include <bx/math.h>
#include <bx/mutex.h>
#include <bx/os.h>
#include <bx/readerwriter.h>
#include <bx/ringbuffer.h>
#include <bx/sort.h>
#include <bx/string.h>
#include <bx/thread.h>
#include <bx/timer.h>
#include <bx/uint32_t.h>

#include "mygfx/platform.h"
#include "bimg/bimg.h"
#include "shader.h"
#include "vertexdecl.h"

#define MYGFX_CHUNK_MAGIC_TEX BX_MAKEFOURCC('T', 'E', 'X', 0x0)

#define MYGFX_CLEAR_COLOR_USE_PALETTE UINT16_C(0x8000)
#define MYGFX_CLEAR_MASK (0                 \
			| MYGFX_CLEAR_COLOR             \
			| MYGFX_CLEAR_DEPTH             \
			| MYGFX_CLEAR_STENCIL           \
			| MYGFX_CLEAR_COLOR_USE_PALETTE \
			)

//stl
#	include <list>
#	include <string>
#	include <unordered_map>
#	include <unordered_set>
#	include <vector>
namespace stl = std;

#if BX_PLATFORM_ANDROID
#	include <android/native_window.h>
#elif BX_PLATFORM_WINDOWS
#	include <windows.h>
#endif // BX_PLATFORM_*

#define MYGFX_MAX_COMPUTE_BINDINGS MYGFX_CONFIG_MAX_TEXTURE_SAMPLERS

#define MYGFX_SAMPLER_INTERNAL_DEFAULT       UINT32_C(0x10000000)
#define MYGFX_SAMPLER_INTERNAL_SHARED        UINT32_C(0x20000000)

#define MYGFX_RESET_INTERNAL_FORCE           UINT32_C(0x80000000)

#define MYGFX_STATE_INTERNAL_SCISSOR         UINT64_C(0x2000000000000000)
#define MYGFX_STATE_INTERNAL_OCCLUSION_QUERY UINT64_C(0x4000000000000000)

#define MYGFX_SUBMIT_INTERNAL_OCCLUSION_VISIBLE UINT8_C(0x80)

#define MYGFX_RENDERER_DIRECT3D9_NAME  "Direct3D 9"
#define MYGFX_RENDERER_DIRECT3D11_NAME "Direct3D 11"
#define MYGFX_RENDERER_DIRECT3D12_NAME "Direct3D 12"
#define MYGFX_RENDERER_METAL_NAME      "Metal"
#define MYGFX_RENDERER_VULKAN_NAME     "Vulkan"
#define MYGFX_RENDERER_GNM_NAME        "GNM"
#define MYGFX_RENDERER_NOOP_NAME       "Noop"

#if MYGFX_CONFIG_RENDERER_OPENGL
#	if MYGFX_CONFIG_RENDERER_OPENGL >= 31 && MYGFX_CONFIG_RENDERER_OPENGL <= 33
#		if MYGFX_CONFIG_RENDERER_OPENGL == 31
#			define MYGFX_RENDERER_OPENGL_NAME "OpenGL 3.1"
#		elif MYGFX_CONFIG_RENDERER_OPENGL == 32
#			define MYGFX_RENDERER_OPENGL_NAME "OpenGL 3.2"
#		else
#			define MYGFX_RENDERER_OPENGL_NAME "OpenGL 3.3"
#		endif // 31+
#	elif MYGFX_CONFIG_RENDERER_OPENGL >= 40 && MYGFX_CONFIG_RENDERER_OPENGL <= 45
#		if MYGFX_CONFIG_RENDERER_OPENGL == 40
#			define MYGFX_RENDERER_OPENGL_NAME "OpenGL 4.0"
#		elif MYGFX_CONFIG_RENDERER_OPENGL == 41
#			define MYGFX_RENDERER_OPENGL_NAME "OpenGL 4.1"
#		elif MYGFX_CONFIG_RENDERER_OPENGL == 42
#			define MYGFX_RENDERER_OPENGL_NAME "OpenGL 4.2"
#		elif MYGFX_CONFIG_RENDERER_OPENGL == 43
#			define MYGFX_RENDERER_OPENGL_NAME "OpenGL 4.3"
#		elif MYGFX_CONFIG_RENDERER_OPENGL == 44
#			define MYGFX_RENDERER_OPENGL_NAME "OpenGL 4.4"
#		else
#			define MYGFX_RENDERER_OPENGL_NAME "OpenGL 4.5"
#		endif // 40+
#	else
#		define MYGFX_RENDERER_OPENGL_NAME "OpenGL 2.1"
#	endif // MYGFX_CONFIG_RENDERER_OPENGL
#elif MYGFX_CONFIG_RENDERER_OPENGLES
#	if MYGFX_CONFIG_RENDERER_OPENGLES == 30
#		define MYGFX_RENDERER_OPENGL_NAME "OpenGL ES 3.0"
#	elif MYGFX_CONFIG_RENDERER_OPENGLES >= 31
#		define MYGFX_RENDERER_OPENGL_NAME "OpenGL ES 3.1"
#	else
#		define MYGFX_RENDERER_OPENGL_NAME "OpenGL ES 2.0"
#	endif // MYGFX_CONFIG_RENDERER_OPENGLES
#else
#	define MYGFX_RENDERER_OPENGL_NAME "OpenGL"
#endif //

namespace mygfx
{
	extern InternalData g_internalData;
	extern PlatformData g_platformData;
	extern bool g_platformDataChangedSinceReset;

#if BGFX_CONFIG_MAX_DRAW_CALLS < (64<<10)
	typedef uint16_t RenderItemCount;
#else
	typedef uint32_t RenderItemCount;
#endif // BGFX_CONFIG_MAX_DRAW_CALLS < (64<<10)

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

	inline bool isValid(const VertexDecl& _decl)
	{
		return 0 != _decl.m_stride;
	}

	struct Condition
	{
		enum Enum
		{
			LessEqual,
			GreaterEqual,
		};
	};

	bool windowsVersionIs(Condition::Enum _op, uint32_t _version);

	constexpr bool isShaderType(uint32_t _magic, char _type)
	{
		return uint32_t(_type) == (_magic & BX_MAKEFOURCC(0xff, 0, 0, 0));
	}

	inline bool isShaderBin(uint32_t _magic)
	{
		return BX_MAKEFOURCC(0, 'S', 'H', 0) == (_magic & BX_MAKEFOURCC(0, 0xff, 0xff, 0))
			&& (isShaderType(_magic, 'C') || isShaderType(_magic, 'F') || isShaderType(_magic, 'V'))
			;
	}

	inline bool isShaderVerLess(uint32_t _magic, uint8_t _version)
	{
		return (_magic & BX_MAKEFOURCC(0, 0, 0, 0xff)) < BX_MAKEFOURCC(0, 0, 0, _version);
	}

	const char* getShaderTypeName(uint32_t _magic);

}

#endif