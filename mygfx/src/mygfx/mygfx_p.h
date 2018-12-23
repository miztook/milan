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

	struct RendererContextI;

	extern void blit(RendererContextI* _renderCtx, TextVideoMemBlitter& _blitter, const TextVideoMem& _mem);

	inline void blit(RendererContextI* _renderCtx, TextVideoMemBlitter& _blitter, const TextVideoMem* _mem)
	{
		blit(_renderCtx, _blitter, *_mem);
	}

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

	struct PredefinedUniform
	{
		enum Enum
		{
			ViewRect,
			ViewTexel,
			View,
			InvView,
			Proj,
			InvProj,
			ViewProj,
			InvViewProj,
			Model,
			ModelView,
			ModelViewProj,
			AlphaRef,
			Count
		};

		uint32_t m_loc;
		uint16_t m_count;
		uint8_t m_type;
	};

	const char* getUniformTypeName(UniformType::Enum _enum);
	UniformType::Enum nameToUniformTypeEnum(const char* _name);
	const char* getPredefinedUniformName(PredefinedUniform::Enum _enum);
	PredefinedUniform::Enum nameToPredefinedUniformEnum(const char* _name);

	class CommandBuffer
	{
		BX_CLASS(CommandBuffer
			, NO_COPY
			, NO_ASSIGNMENT
		);

	public:
		CommandBuffer()
			: m_pos(0)
			, m_size(MYGFX_CONFIG_MAX_COMMAND_BUFFER_SIZE)
		{
			finish();
		}

		enum Enum
		{
			RendererInit,
			RendererShutdownBegin,
			CreateVertexDecl,
			CreateIndexBuffer,
			CreateVertexBuffer,
			CreateDynamicIndexBuffer,
			UpdateDynamicIndexBuffer,
			CreateDynamicVertexBuffer,
			UpdateDynamicVertexBuffer,
			CreateShader,
			CreateProgram,
			CreateTexture,
			UpdateTexture,
			ResizeTexture,
			CreateFrameBuffer,
			CreateUniform,
			UpdateViewName,
			InvalidateOcclusionQuery,
			SetName,
			End,
			RendererShutdownEnd,
			DestroyVertexDecl,
			DestroyIndexBuffer,
			DestroyVertexBuffer,
			DestroyDynamicIndexBuffer,
			DestroyDynamicVertexBuffer,
			DestroyShader,
			DestroyProgram,
			DestroyTexture,
			DestroyFrameBuffer,
			DestroyUniform,
			ReadTexture,
			RequestScreenShot,
		};

		void write(const void* _data, uint32_t _size)
		{
			BX_CHECK(m_size == MYGFX_CONFIG_MAX_COMMAND_BUFFER_SIZE, "Called write outside start/finish?");
			BX_CHECK(m_pos < m_size, "CommandBuffer::write error (pos: %d, size: %d).", m_pos, m_size);
			bx::memCopy(&m_buffer[m_pos], _data, _size);
			m_pos += _size;
		}

		template<typename Type>
		void write(const Type& _in)
		{
			align(BX_ALIGNOF(Type));
			write(reinterpret_cast<const uint8_t*>(&_in), sizeof(Type));
		}

		void read(void* _data, uint32_t _size)
		{
			BX_CHECK(m_pos < m_size, "CommandBuffer::read error (pos: %d, size: %d).", m_pos, m_size);
			bx::memCopy(_data, &m_buffer[m_pos], _size);
			m_pos += _size;
		}

		template<typename Type>
		void read(Type& _in)
		{
			align(BX_ALIGNOF(Type));
			read(reinterpret_cast<uint8_t*>(&_in), sizeof(Type));
		}

		const uint8_t* skip(uint32_t _size)
		{
			BX_CHECK(m_pos < m_size, "CommandBuffer::skip error (pos: %d, size: %d).", m_pos, m_size);
			const uint8_t* result = &m_buffer[m_pos];
			m_pos += _size;
			return result;
		}

		template<typename Type>
		void skip()
		{
			align(BX_ALIGNOF(Type));
			skip(sizeof(Type));
		}

		void align(uint32_t _alignment)
		{
			const uint32_t mask = _alignment - 1;
			const uint32_t pos = (m_pos + mask) & (~mask);
			m_pos = pos;
		}

		void reset()
		{
			m_pos = 0;
		}

		void start()
		{
			m_pos = 0;
			m_size = MYGFX_CONFIG_MAX_COMMAND_BUFFER_SIZE;
		}

		void finish()
		{
			uint8_t cmd = End;
			write(cmd);
			m_size = m_pos;
			m_pos = 0;
		}

		uint32_t m_pos;
		uint32_t m_size;
		uint8_t m_buffer[MYGFX_CONFIG_MAX_COMMAND_BUFFER_SIZE];
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

#define MYGFX_UNIFORM_FRAGMENTBIT UINT8_C(0x10)
#define MYGFX_UNIFORM_SAMPLERBIT  UINT8_C(0x20)
#define MYGFX_UNIFORM_MASK (MYGFX_UNIFORM_FRAGMENTBIT|MYGFX_UNIFORM_SAMPLERBIT)

	class UniformBuffer
	{
	public:
		static UniformBuffer* create(uint32_t _size = 1 << 20)
		{
			const uint32_t structSize = sizeof(UniformBuffer) - sizeof(UniformBuffer::m_buffer);

			uint32_t size = BX_ALIGN_16(_size);
			void*    data = BX_ALLOC(g_allocator, size + structSize);
			return BX_PLACEMENT_NEW(data, UniformBuffer)(size);
		}

		static void destroy(UniformBuffer* _uniformBuffer)
		{
			_uniformBuffer->~UniformBuffer();
			BX_FREE(g_allocator, _uniformBuffer);
		}

		static void update(UniformBuffer** _uniformBuffer, uint32_t _treshold = 64 << 10, uint32_t _grow = 1 << 20)
		{
			UniformBuffer* uniformBuffer = *_uniformBuffer;
			if (_treshold >= uniformBuffer->m_size - uniformBuffer->m_pos)
			{
				const uint32_t structSize = sizeof(UniformBuffer) - sizeof(UniformBuffer::m_buffer);
				uint32_t size = BX_ALIGN_16(uniformBuffer->m_size + _grow);
				void*    data = BX_REALLOC(g_allocator, uniformBuffer, size + structSize);
				uniformBuffer = reinterpret_cast<UniformBuffer*>(data);
				uniformBuffer->m_size = size;

				*_uniformBuffer = uniformBuffer;
			}
		}

		static uint32_t encodeOpcode(UniformType::Enum _type, uint16_t _loc, uint16_t _num, uint16_t _copy)
		{
			const uint32_t type = _type << CONSTANT_OPCODE_TYPE_SHIFT;
			const uint32_t loc = _loc << CONSTANT_OPCODE_LOC_SHIFT;
			const uint32_t num = _num << CONSTANT_OPCODE_NUM_SHIFT;
			const uint32_t copy = _copy << CONSTANT_OPCODE_COPY_SHIFT;
			return type | loc | num | copy;
		}

		static void decodeOpcode(uint32_t _opcode, UniformType::Enum& _type, uint16_t& _loc, uint16_t& _num, uint16_t& _copy)
		{
			const uint32_t type = (_opcode&CONSTANT_OPCODE_TYPE_MASK) >> CONSTANT_OPCODE_TYPE_SHIFT;
			const uint32_t loc = (_opcode&CONSTANT_OPCODE_LOC_MASK) >> CONSTANT_OPCODE_LOC_SHIFT;
			const uint32_t num = (_opcode&CONSTANT_OPCODE_NUM_MASK) >> CONSTANT_OPCODE_NUM_SHIFT;
			const uint32_t copy = (_opcode&CONSTANT_OPCODE_COPY_MASK); // >> CONSTANT_OPCODE_COPY_SHIFT;

			_type = (UniformType::Enum)(type);
			_copy = (uint16_t)copy;
			_num = (uint16_t)num;
			_loc = (uint16_t)loc;
		}

		void write(const void* _data, uint32_t _size)
		{
			BX_CHECK(m_pos + _size < m_size, "Write would go out of bounds. pos %d + size %d > max size: %d).", m_pos, _size, m_size);

			if (m_pos + _size < m_size)
			{
				bx::memCopy(&m_buffer[m_pos], _data, _size);
				m_pos += _size;
			}
		}

		void write(uint32_t _value)
		{
			write(&_value, sizeof(uint32_t));
		}

		const char* read(uint32_t _size)
		{
			BX_CHECK(m_pos < m_size, "Out of bounds %d (size: %d).", m_pos, m_size);
			const char* result = &m_buffer[m_pos];
			m_pos += _size;
			return result;
		}

		uint32_t read()
		{
			uint32_t result;
			bx::memCopy(&result, read(sizeof(uint32_t)), sizeof(uint32_t));
			return result;
		}

		bool isEmpty() const
		{
			return 0 == m_pos;
		}

		uint32_t getPos() const
		{
			return m_pos;
		}

		void reset(uint32_t _pos = 0)
		{
			m_pos = _pos;
		}

		void finish()
		{
			write(UniformType::End);
			m_pos = 0;
		}

		void writeUniform(UniformType::Enum _type, uint16_t _loc, const void* _value, uint16_t _num = 1);
		void writeUniformHandle(UniformType::Enum _type, uint16_t _loc, UniformHandle _handle, uint16_t _num = 1);
		void writeMarker(const char* _marker);

	private:
		UniformBuffer(uint32_t _size)
			: m_size(_size)
			, m_pos(0)
		{
			finish();
		}

		~UniformBuffer()
		{
		}

		uint32_t m_size;
		uint32_t m_pos;
		char m_buffer[256 << 20];
	};

	struct UniformRegInfo
	{
		UniformHandle m_handle;
	};

	class UniformRegistry
	{
	public:
		UniformRegistry()
		{
		}

		~UniformRegistry()
		{
		}

		const UniformRegInfo* find(const char* _name) const
		{
			uint16_t handle = m_uniforms.find(bx::hash<bx::HashMurmur2A>(_name));
			if (kInvalidHandle != handle)
			{
				return &m_info[handle];
			}

			return NULL;
		}

		const UniformRegInfo& add(UniformHandle _handle, const char* _name)
		{
			BX_CHECK(isValid(_handle), "Uniform handle is invalid (name: %s)!", _name);
			const uint32_t key = bx::hash<bx::HashMurmur2A>(_name);
			m_uniforms.removeByKey(key);
			m_uniforms.insert(key, _handle.idx);

			UniformRegInfo& info = m_info[_handle.idx];
			info.m_handle = _handle;

			return info;
		}

		void remove(UniformHandle _handle)
		{
			m_uniforms.removeByHandle(_handle.idx);
		}

	private:
		typedef bx::HandleHashMapT<MYGFX_CONFIG_MAX_UNIFORMS * 2> UniformHashMap;
		UniformHashMap m_uniforms;
		UniformRegInfo m_info[MYGFX_CONFIG_MAX_UNIFORMS];
	};

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

	// First-fit non-local allocator.
	class NonLocalAllocator
	{
	public:
		static const uint64_t kInvalidBlock = UINT64_MAX;

		NonLocalAllocator()
		{
		}

		~NonLocalAllocator()
		{
		}

		void reset()
		{
			m_free.clear();
			m_used.clear();
		}

		void add(uint64_t _ptr, uint32_t _size)
		{
			m_free.push_back(Free(_ptr, _size));
		}

		uint64_t remove()
		{
			BX_CHECK(0 == m_used.size(), "");

			if (0 < m_free.size())
			{
				Free freeBlock = m_free.front();
				m_free.pop_front();
				return freeBlock.m_ptr;
			}

			return 0;
		}

		uint64_t alloc(uint32_t _size)
		{
			_size = bx::max(_size, 16u);

			for (FreeList::iterator it = m_free.begin(), itEnd = m_free.end(); it != itEnd; ++it)
			{
				if (it->m_size >= _size)
				{
					uint64_t ptr = it->m_ptr;

					m_used.insert(stl::make_pair(ptr, _size));

					if (it->m_size != _size)
					{
						it->m_size -= _size;
						it->m_ptr += _size;
					}
					else
					{
						m_free.erase(it);
					}

					return ptr;
				}
			}

			// there is no block large enough.
			return kInvalidBlock;
		}

		void free(uint64_t _block)
		{
			UsedList::iterator it = m_used.find(_block);
			if (it != m_used.end())
			{
				m_free.push_front(Free(it->first, it->second));
				m_used.erase(it);
			}
		}

		bool compact()
		{
			m_free.sort();

			for (FreeList::iterator it = m_free.begin(), next = it, itEnd = m_free.end(); next != itEnd;)
			{
				if ((it->m_ptr + it->m_size) == next->m_ptr)
				{
					it->m_size += next->m_size;
					next = m_free.erase(next);
				}
				else
				{
					it = next;
					++next;
				}
			}

			return 0 == m_used.size();
		}

	private:
		struct Free
		{
			Free(uint64_t _ptr, uint32_t _size)
				: m_ptr(_ptr)
				, m_size(_size)
			{
			}

			bool operator<(const Free& rhs) const
			{
				return m_ptr < rhs.m_ptr;
			}

			uint64_t m_ptr;
			uint32_t m_size;
		};

		typedef stl::list<Free> FreeList;
		FreeList m_free;

		typedef stl::unordered_map<uint64_t, uint32_t> UsedList;
		UsedList m_used;
	};

	struct BX_NO_VTABLE RendererContextI
	{
		virtual ~RendererContextI() {}
		virtual RendererType::Enum getRendererType() const = 0;
		virtual const char* getRendererName() const = 0;
		virtual bool isDeviceRemoved() = 0;
		virtual void flip() = 0;
		virtual void createIndexBuffer(IndexBufferHandle _handle, const Memory* _mem, uint16_t _flags) = 0;
		virtual void destroyIndexBuffer(IndexBufferHandle _handle) = 0;
		virtual void createVertexDecl(VertexDeclHandle _handle, const VertexDecl& _decl) = 0;
		virtual void destroyVertexDecl(VertexDeclHandle _handle) = 0;
		virtual void createVertexBuffer(VertexBufferHandle _handle, const Memory* _mem, VertexDeclHandle _declHandle, uint16_t _flags) = 0;
		virtual void destroyVertexBuffer(VertexBufferHandle _handle) = 0;
		virtual void createDynamicIndexBuffer(IndexBufferHandle _handle, uint32_t _size, uint16_t _flags) = 0;
		virtual void updateDynamicIndexBuffer(IndexBufferHandle _handle, uint32_t _offset, uint32_t _size, const Memory* _mem) = 0;
		virtual void destroyDynamicIndexBuffer(IndexBufferHandle _handle) = 0;
		virtual void createDynamicVertexBuffer(VertexBufferHandle _handle, uint32_t _size, uint16_t _flags) = 0;
		virtual void updateDynamicVertexBuffer(VertexBufferHandle _handle, uint32_t _offset, uint32_t _size, const Memory* _mem) = 0;
		virtual void destroyDynamicVertexBuffer(VertexBufferHandle _handle) = 0;
		virtual void createShader(ShaderHandle _handle, const Memory* _mem) = 0;
		virtual void destroyShader(ShaderHandle _handle) = 0;
		virtual void createProgram(ProgramHandle _handle, ShaderHandle _vsh, ShaderHandle _fsh) = 0;
		virtual void destroyProgram(ProgramHandle _handle) = 0;
		virtual void* createTexture(TextureHandle _handle, const Memory* _mem, uint64_t _flags, uint8_t _skip) = 0;
		virtual void updateTextureBegin(TextureHandle _handle, uint8_t _side, uint8_t _mip) = 0;
		virtual void updateTexture(TextureHandle _handle, uint8_t _side, uint8_t _mip, const Rect& _rect, uint16_t _z, uint16_t _depth, uint16_t _pitch, const Memory* _mem) = 0;
		virtual void updateTextureEnd() = 0;
		virtual void readTexture(TextureHandle _handle, void* _data, uint8_t _mip) = 0;
		virtual void resizeTexture(TextureHandle _handle, uint16_t _width, uint16_t _height, uint8_t _numMips, uint16_t _numLayers) = 0;
		virtual void overrideInternal(TextureHandle _handle, uintptr_t _ptr) = 0;
		virtual uintptr_t getInternal(TextureHandle _handle) = 0;
		virtual void destroyTexture(TextureHandle _handle) = 0;
		virtual void createFrameBuffer(FrameBufferHandle _handle, uint8_t _num, const Attachment* _attachment) = 0;
		virtual void createFrameBuffer(FrameBufferHandle _handle, void* _nwh, uint32_t _width, uint32_t _height, TextureFormat::Enum _format, TextureFormat::Enum _depthFormat) = 0;
		virtual void destroyFrameBuffer(FrameBufferHandle _handle) = 0;
		virtual void createUniform(UniformHandle _handle, UniformType::Enum _type, uint16_t _num, const char* _name) = 0;
		virtual void destroyUniform(UniformHandle _handle) = 0;
		virtual void requestScreenShot(FrameBufferHandle _handle, const char* _filePath) = 0;
		virtual void updateViewName(ViewId _id, const char* _name) = 0;
		virtual void updateUniform(uint16_t _loc, const void* _data, uint32_t _size) = 0;
		virtual void setMarker(const char* _marker, uint32_t _size) = 0;
		virtual void invalidateOcclusionQuery(OcclusionQueryHandle _handle) = 0;
		virtual void setName(Handle _handle, const char* _name) = 0;
		virtual void submit(Frame* _render, ClearQuad& _clearQuad, TextVideoMemBlitter& _textVideoMemBlitter) = 0;
		virtual void blitSetup(TextVideoMemBlitter& _blitter) = 0;
		virtual void blitRender(TextVideoMemBlitter& _blitter, uint32_t _numIndices) = 0;
	};

	void rendererUpdateUniforms(RendererContextI* _renderCtx, UniformBuffer* _uniformBuffer, uint32_t _begin, uint32_t _end);

}

#endif