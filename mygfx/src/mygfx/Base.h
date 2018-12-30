#pragma once

#include "mygfx/mygfx.h"
#include "config.h"

#include <inttypes.h>

#ifndef MYGFX_CONFIG_DEBUG
#	define MYGFX_CONFIG_DEBUG 0
#endif // MYGFX_CONFIG_DEBUG

#if MYGFX_CONFIG_DEBUG || BX_COMPILER_CLANG_ANALYZER
#	define BX_TRACE _BX_TRACE
#	define BX_WARN  _BX_WARN
#	define BX_CHECK _BX_CHECK
#	define BX_CONFIG_ALLOCATOR_DEBUG 1
#endif // MYGFX_CONFIG_DEBUG

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

#if MYGFX_CONFIG_MAX_DRAW_CALLS < (64<<10)
	typedef uint16_t RenderItemCount;
#else
	typedef uint32_t RenderItemCount;
#endif // BGFX_CONFIG_MAX_DRAW_CALLS < (64<<10)

	struct Condition
	{
		enum Enum
		{
			LessEqual,
			GreaterEqual,
		};
	};

	inline bool isValid(const VertexDecl& _decl)
	{
		return 0 != _decl.m_stride;
	}

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

	extern const uint32_t g_uniformTypeSize[UniformType::Count + 1];
	extern CallbackI* g_callback;
	extern bx::AllocatorI* g_allocator;
	extern Caps g_caps;

	typedef bx::StringT<&g_allocator> String;

	void setGraphicsDebuggerPresent(bool _present);
	bool isGraphicsDebuggerPresent();
	void release(const Memory* _mem);
	const char* getAttribName(Attrib::Enum _attr);
	const char* getAttribNameShort(Attrib::Enum _attr);
	void getTextureSizeFromRatio(BackbufferRatio::Enum _ratio, uint16_t& _width, uint16_t& _height);
	TextureFormat::Enum getViableTextureFormat(const bimg::ImageContainer& _imageContainer);
	const char* getName(TextureFormat::Enum _fmt);
	const char* getName(UniformHandle _handle);
	const char* getName(Topology::Enum _topology);

	template<typename Ty>
	inline void release(Ty)
	{
	}

	template<>
	inline void release(Memory* _mem)
	{
		release((const Memory*)_mem);
	}

	inline uint32_t castfu(float _value)
	{
		union { float fl; uint32_t ui; } un;
		un.fl = _value;
		return un.ui;
	}

	inline uint64_t packStencil(uint32_t _fstencil, uint32_t _bstencil)
	{
		return (uint64_t(_bstencil) << 32) | uint64_t(_fstencil);
	}

	inline uint32_t unpackStencil(uint8_t _0or1, uint64_t _stencil)
	{
		return uint32_t((_stencil >> (32 * _0or1)));
	}

	inline bool needBorderColor(uint32_t _flags)
	{
		return MYGFX_SAMPLER_U_BORDER == (_flags & MYGFX_SAMPLER_U_BORDER)
			|| MYGFX_SAMPLER_V_BORDER == (_flags & MYGFX_SAMPLER_V_BORDER)
			|| MYGFX_SAMPLER_W_BORDER == (_flags & MYGFX_SAMPLER_W_BORDER)
			;
	}

	inline uint8_t calcNumMips(bool _hasMips, uint16_t _width, uint16_t _height, uint16_t _depth = 1)
	{
		if (_hasMips)
		{
			const uint32_t max = bx::max(_width, _height, _depth);
			const uint32_t num = 1 + uint32_t(bx::log2(float(max)));

			return uint8_t(num);
		}

		return 1;
	}

	void dump(const VertexDecl& _decl);

	

	//
#define SORT_KEY_NUM_BITS_VIEW         10

#define SORT_KEY_VIEW_SHIFT            (64-SORT_KEY_NUM_BITS_VIEW)
#define SORT_KEY_VIEW_MASK             ( (uint64_t(MYGFX_CONFIG_MAX_VIEWS-1) )<<SORT_KEY_VIEW_SHIFT)

#define SORT_KEY_DRAW_BIT_SHIFT        (SORT_KEY_VIEW_SHIFT - 1)
#define SORT_KEY_DRAW_BIT              (UINT64_C(1)<<SORT_KEY_DRAW_BIT_SHIFT)

	//
#define SORT_KEY_NUM_BITS_DRAW_TYPE    2

#define SORT_KEY_DRAW_TYPE_BIT_SHIFT   (SORT_KEY_DRAW_BIT_SHIFT - SORT_KEY_NUM_BITS_DRAW_TYPE)
#define SORT_KEY_DRAW_TYPE_MASK        (UINT64_C(3)<<SORT_KEY_DRAW_TYPE_BIT_SHIFT)

#define SORT_KEY_DRAW_TYPE_PROGRAM     (UINT64_C(0)<<SORT_KEY_DRAW_TYPE_BIT_SHIFT)
#define SORT_KEY_DRAW_TYPE_DEPTH       (UINT64_C(1)<<SORT_KEY_DRAW_TYPE_BIT_SHIFT)
#define SORT_KEY_DRAW_TYPE_SEQUENCE    (UINT64_C(2)<<SORT_KEY_DRAW_TYPE_BIT_SHIFT)

	//
#define SORT_KEY_NUM_BITS_TRANS        2

#define SORT_KEY_DRAW_0_TRANS_SHIFT    (SORT_KEY_DRAW_TYPE_BIT_SHIFT - SORT_KEY_NUM_BITS_TRANS)
#define SORT_KEY_DRAW_0_TRANS_MASK     (UINT64_C(0x3)<<SORT_KEY_DRAW_0_TRANS_SHIFT)

#define SORT_KEY_DRAW_0_PROGRAM_SHIFT  (SORT_KEY_DRAW_0_TRANS_SHIFT - MYGFX_CONFIG_SORT_KEY_NUM_BITS_PROGRAM)
#define SORT_KEY_DRAW_0_PROGRAM_MASK   ( (uint64_t(MYGFX_CONFIG_MAX_PROGRAMS-1) )<<SORT_KEY_DRAW_0_PROGRAM_SHIFT)

#define SORT_KEY_DRAW_0_DEPTH_SHIFT    (SORT_KEY_DRAW_0_PROGRAM_SHIFT - MYGFX_CONFIG_SORT_KEY_NUM_BITS_DEPTH)
#define SORT_KEY_DRAW_0_DEPTH_MASK     ( ( (UINT64_C(1)<<MYGFX_CONFIG_SORT_KEY_NUM_BITS_DEPTH)-1)<<SORT_KEY_DRAW_0_DEPTH_SHIFT)

	//
#define SORT_KEY_DRAW_1_DEPTH_SHIFT    (SORT_KEY_DRAW_TYPE_BIT_SHIFT - MYGFX_CONFIG_SORT_KEY_NUM_BITS_DEPTH)
#define SORT_KEY_DRAW_1_DEPTH_MASK     ( ( (UINT64_C(1)<<MYGFX_CONFIG_SORT_KEY_NUM_BITS_DEPTH)-1)<<SORT_KEY_DRAW_1_DEPTH_SHIFT)

#define SORT_KEY_DRAW_1_TRANS_SHIFT    (SORT_KEY_DRAW_1_DEPTH_SHIFT - SORT_KEY_NUM_BITS_TRANS)
#define SORT_KEY_DRAW_1_TRANS_MASK     (UINT64_C(0x3)<<SORT_KEY_DRAW_1_TRANS_SHIFT)

#define SORT_KEY_DRAW_1_PROGRAM_SHIFT  (SORT_KEY_DRAW_1_TRANS_SHIFT - MYGFX_CONFIG_SORT_KEY_NUM_BITS_PROGRAM)
#define SORT_KEY_DRAW_1_PROGRAM_MASK   ( (uint64_t(MYGFX_CONFIG_MAX_PROGRAMS-1) )<<SORT_KEY_DRAW_1_PROGRAM_SHIFT)

	//
#define SORT_KEY_DRAW_2_SEQ_SHIFT      (SORT_KEY_DRAW_TYPE_BIT_SHIFT - MYGFX_CONFIG_SORT_KEY_NUM_BITS_SEQ)
#define SORT_KEY_DRAW_2_SEQ_MASK       ( ( (UINT64_C(1)<<MYGFX_CONFIG_SORT_KEY_NUM_BITS_SEQ)-1)<<SORT_KEY_DRAW_2_SEQ_SHIFT)

#define SORT_KEY_DRAW_2_TRANS_SHIFT    (SORT_KEY_DRAW_2_SEQ_SHIFT - SORT_KEY_NUM_BITS_TRANS)
#define SORT_KEY_DRAW_2_TRANS_MASK     (UINT64_C(0x3)<<SORT_KEY_DRAW_2_TRANS_SHIFT)

#define SORT_KEY_DRAW_2_PROGRAM_SHIFT  (SORT_KEY_DRAW_2_TRANS_SHIFT - MYGFX_CONFIG_SORT_KEY_NUM_BITS_PROGRAM)
#define SORT_KEY_DRAW_2_PROGRAM_MASK   ( (uint64_t(MYGFX_CONFIG_MAX_PROGRAMS-1) )<<SORT_KEY_DRAW_2_PROGRAM_SHIFT)

	//
#define SORT_KEY_COMPUTE_SEQ_SHIFT     (SORT_KEY_DRAW_BIT_SHIFT - MYGFX_CONFIG_SORT_KEY_NUM_BITS_SEQ)
#define SORT_KEY_COMPUTE_SEQ_MASK      ( ( (UINT64_C(1)<<MYGFX_CONFIG_SORT_KEY_NUM_BITS_SEQ)-1)<<SORT_KEY_COMPUTE_SEQ_SHIFT)

#define SORT_KEY_COMPUTE_PROGRAM_SHIFT (SORT_KEY_COMPUTE_SEQ_SHIFT - MYGFX_CONFIG_SORT_KEY_NUM_BITS_PROGRAM)
#define SORT_KEY_COMPUTE_PROGRAM_MASK  ( (uint64_t(MYGFX_CONFIG_MAX_PROGRAMS-1) )<<SORT_KEY_COMPUTE_PROGRAM_SHIFT)


	BX_STATIC_ASSERT(MYGFX_CONFIG_MAX_VIEWS <= (1 << SORT_KEY_NUM_BITS_VIEW));
	BX_STATIC_ASSERT((MYGFX_CONFIG_MAX_PROGRAMS & (MYGFX_CONFIG_MAX_PROGRAMS - 1)) == 0); // Must be power of 2.
	BX_STATIC_ASSERT((0 // Render key mask shouldn't overlap.
		| SORT_KEY_VIEW_MASK
		| SORT_KEY_DRAW_BIT
		| SORT_KEY_DRAW_TYPE_MASK
		| SORT_KEY_DRAW_0_TRANS_MASK
		| SORT_KEY_DRAW_0_PROGRAM_MASK
		| SORT_KEY_DRAW_0_DEPTH_MASK
		) == (0
			^ SORT_KEY_VIEW_MASK
			^ SORT_KEY_DRAW_BIT
			^ SORT_KEY_DRAW_TYPE_MASK
			^ SORT_KEY_DRAW_0_TRANS_MASK
			^ SORT_KEY_DRAW_0_PROGRAM_MASK
			^ SORT_KEY_DRAW_0_DEPTH_MASK
			));
	BX_STATIC_ASSERT((0 // Render key mask shouldn't overlap.
		| SORT_KEY_VIEW_MASK
		| SORT_KEY_DRAW_BIT
		| SORT_KEY_DRAW_TYPE_MASK
		| SORT_KEY_DRAW_1_DEPTH_MASK
		| SORT_KEY_DRAW_1_TRANS_MASK
		| SORT_KEY_DRAW_1_PROGRAM_MASK
		) == (0
			^ SORT_KEY_VIEW_MASK
			^ SORT_KEY_DRAW_BIT
			^ SORT_KEY_DRAW_TYPE_MASK
			^ SORT_KEY_DRAW_1_DEPTH_MASK
			^ SORT_KEY_DRAW_1_TRANS_MASK
			^ SORT_KEY_DRAW_1_PROGRAM_MASK
			));
	BX_STATIC_ASSERT((0 // Render key mask shouldn't overlap.
		| SORT_KEY_VIEW_MASK
		| SORT_KEY_DRAW_BIT
		| SORT_KEY_DRAW_TYPE_MASK
		| SORT_KEY_DRAW_2_SEQ_MASK
		| SORT_KEY_DRAW_2_TRANS_MASK
		| SORT_KEY_DRAW_2_PROGRAM_MASK
		) == (0
			^ SORT_KEY_VIEW_MASK
			^ SORT_KEY_DRAW_BIT
			^ SORT_KEY_DRAW_TYPE_MASK
			^ SORT_KEY_DRAW_2_SEQ_MASK
			^ SORT_KEY_DRAW_2_TRANS_MASK
			^ SORT_KEY_DRAW_2_PROGRAM_MASK
			));
	BX_STATIC_ASSERT((0 // Compute key mask shouldn't overlap.
		| SORT_KEY_VIEW_MASK
		| SORT_KEY_DRAW_BIT
		| SORT_KEY_COMPUTE_SEQ_SHIFT
		| SORT_KEY_COMPUTE_PROGRAM_MASK
		) == (0
			^ SORT_KEY_VIEW_MASK
			^ SORT_KEY_DRAW_BIT
			^ SORT_KEY_COMPUTE_SEQ_SHIFT
			^ SORT_KEY_COMPUTE_PROGRAM_MASK
			));

	




}