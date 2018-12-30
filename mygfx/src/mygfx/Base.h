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

	struct TextVideoMem
	{
		TextVideoMem()
			: m_mem(NULL)
			, m_size(0)
			, m_width(0)
			, m_height(0)
			, m_small(false)
		{
			resize(false, 1, 1);
			clear();
		}

		~TextVideoMem()
		{
			BX_FREE(g_allocator, m_mem);
		}

		void resize(bool _small, uint32_t _width, uint32_t _height)
		{
			uint32_t width = bx::uint32_imax(1, _width / 8);
			uint32_t height = bx::uint32_imax(1, _height / (_small ? 8 : 16));

			if (NULL == m_mem
				|| m_width != width
				|| m_height != height
				|| m_small != _small)
			{
				m_small = _small;
				m_width = (uint16_t)width;
				m_height = (uint16_t)height;

				uint32_t size = m_size;
				m_size = m_width * m_height;

				m_mem = (MemSlot*)BX_REALLOC(g_allocator, m_mem, m_size * sizeof(MemSlot));

				if (size < m_size)
				{
					bx::memSet(&m_mem[size], 0, (m_size - size) * sizeof(MemSlot));
				}
			}
		}

		void clear(uint8_t _attr = 0)
		{
			MemSlot* mem = m_mem;
			bx::memSet(mem, 0, m_size * sizeof(MemSlot));
			if (_attr != 0)
			{
				for (uint32_t ii = 0, num = m_size; ii < num; ++ii)
				{
					mem[ii].attribute = _attr;
				}
			}
		}

		void printfVargs(uint16_t _x, uint16_t _y, uint8_t _attr, const char* _format, va_list _argList);

		void printf(uint16_t _x, uint16_t _y, uint8_t _attr, const char* _format, ...)
		{
			va_list argList;
			va_start(argList, _format);
			printfVargs(_x, _y, _attr, _format, argList);
			va_end(argList);
		}

		void image(uint16_t _x, uint16_t _y, uint16_t _width, uint16_t _height, const void* _data, uint16_t _pitch)
		{
			if (_x < m_width && _y < m_height)
			{
				MemSlot* dst = &m_mem[_y*m_width + _x];
				const uint8_t* src = (const uint8_t*)_data;
				const uint32_t width = bx::min<uint32_t>(m_width, _width + _x) - _x;
				const uint32_t height = bx::min<uint32_t>(m_height, _height + _y) - _y;
				const uint32_t dstPitch = m_width;

				for (uint32_t ii = 0; ii < height; ++ii)
				{
					for (uint32_t jj = 0; jj < width; ++jj)
					{
						dst[jj].character = src[jj * 2];
						dst[jj].attribute = src[jj * 2 + 1];
					}

					src += _pitch;
					dst += dstPitch;
				}
			}
		}

		struct MemSlot
		{
			uint8_t attribute;
			uint8_t character;
		};

		MemSlot* m_mem;
		uint32_t m_size;
		uint16_t m_width;
		uint16_t m_height;
		bool m_small;
	};

	struct TextVideoMemBlitter
	{
		void init();
		void shutdown();

		TextureHandle m_texture;
		TransientVertexBuffer* m_vb;
		TransientIndexBuffer* m_ib;
		VertexDecl m_decl;
		ProgramHandle m_program;
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

	struct ClearQuad
	{
		ClearQuad()
		{
			for (uint32_t ii = 0; ii < BX_COUNTOF(m_program); ++ii)
			{
				m_program[ii].idx = kInvalidHandle;
			}
		}

		void init();
		void shutdown();

		TransientVertexBuffer* m_vb;
		VertexDecl m_decl;
		ProgramHandle m_program[MYGFX_CONFIG_MAX_FRAME_BUFFER_ATTACHMENTS];
	};

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

#define CONSTANT_OPCODE_TYPE_SHIFT 27
#define CONSTANT_OPCODE_TYPE_MASK  UINT32_C(0xf8000000)
#define CONSTANT_OPCODE_LOC_SHIFT  11
#define CONSTANT_OPCODE_LOC_MASK   UINT32_C(0x07fff800)
#define CONSTANT_OPCODE_NUM_SHIFT  1
#define CONSTANT_OPCODE_NUM_MASK   UINT32_C(0x000007fe)
#define CONSTANT_OPCODE_COPY_SHIFT 0
#define CONSTANT_OPCODE_COPY_MASK  UINT32_C(0x00000001)



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