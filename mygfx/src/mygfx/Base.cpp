#include "mygfx_p.h"

namespace mygfx
{
	static bool s_graphicsDebuggerPresent = false;

	void setGraphicsDebuggerPresent(bool _present)
	{
		BX_TRACE("Graphics debugger is %spresent.", _present ? "" : "not ");
		s_graphicsDebuggerPresent = _present;
	}

	bool isGraphicsDebuggerPresent()
	{
		return s_graphicsDebuggerPresent;
	}

	void fatal(const char* _filePath, uint16_t _line, Fatal::Enum _code, const char* _format, ...)
	{
		va_list argList;
		va_start(argList, _format);

		char temp[8192];
		char* out = temp;
		int32_t len = bx::vsnprintf(out, sizeof(temp), _format, argList);
		if ((int32_t)sizeof(temp) < len)
		{
			out = (char*)alloca(len + 1);
			len = bx::vsnprintf(out, len, _format, argList);
		}
		out[len] = '\0';

		if (BX_UNLIKELY(NULL == g_callback))
		{
			bx::debugPrintf("%s(%d): BGFX 0x%08x: %s", _filePath, _line, _code, out);
			abort();
		}
		else
		{
			g_callback->fatal(_filePath, _line, _code, out);
		}

		va_end(argList);
	}

	void trace(const char* _filePath, uint16_t _line, const char* _format, ...)
	{
		va_list argList;
		va_start(argList, _format);

		if (BX_UNLIKELY(NULL == g_callback))
		{
			bx::debugPrintfVargs(_format, argList);
		}
		else
		{
			g_callback->traceVargs(_filePath, _line, _format, argList);
		}

		va_end(argList);
	}

	bool windowsVersionIs(Condition::Enum _op, uint32_t _version)
	{
#if BX_PLATFORM_WINDOWS
		static const uint8_t s_condition[] =
		{
			VER_LESS_EQUAL,
			VER_GREATER_EQUAL,
		};

		OSVERSIONINFOEXA ovi;
		bx::memSet(&ovi, 0, sizeof(ovi));
		ovi.dwOSVersionInfoSize = sizeof(ovi);
		// _WIN32_WINNT_WINBLUE 0x0603
		// _WIN32_WINNT_WIN8    0x0602
		// _WIN32_WINNT_WIN7    0x0601
		// _WIN32_WINNT_VISTA   0x0600
		ovi.dwMajorVersion = HIBYTE(_version);
		ovi.dwMinorVersion = LOBYTE(_version);
		DWORDLONG cond = 0;
		VER_SET_CONDITION(cond, VER_MAJORVERSION, s_condition[_op]);
		VER_SET_CONDITION(cond, VER_MINORVERSION, s_condition[_op]);
		return !!VerifyVersionInfoA(&ovi, VER_MAJORVERSION | VER_MINORVERSION, cond);
#else
		BX_UNUSED(_op, _version);
		return false;
#endif // BX_PLATFORM_WINDOWS
	}

	const char* getName(TextureFormat::Enum _fmt)
	{
		return bimg::getName(bimg::TextureFormat::Enum(_fmt));
	}

	const char* getName(UniformHandle _handle)
	{
		return s_ctx->m_uniformRef[_handle.idx].m_name.getPtr();
	}

	static const char* s_topologyName[] =
	{
		"Triangles",
		"TriStrip",
		"Lines",
		"LineStrip",
		"Points",
	};
	BX_STATIC_ASSERT(Topology::Count == BX_COUNTOF(s_topologyName));

	const char* getName(Topology::Enum _topology)
	{
		return s_topologyName[bx::min(_topology, Topology::PointList)];
	}

	const char* getShaderTypeName(uint32_t _magic)
	{
		if (isShaderType(_magic, 'C'))
		{
			return "Compute";
		}
		else if (isShaderType(_magic, 'F'))
		{
			return "Fragment";
		}
		else if (isShaderType(_magic, 'V'))
		{
			return "Vertex";
		}

		BX_CHECK(false, "Invalid shader type!");

		return NULL;
	}

	void getTextureSizeFromRatio(BackbufferRatio::Enum _ratio, uint16_t& _width, uint16_t& _height)
	{
		switch (_ratio)
		{
		case BackbufferRatio::Half:      _width /= 2; _height /= 2; break;
		case BackbufferRatio::Quarter:   _width /= 4; _height /= 4; break;
		case BackbufferRatio::Eighth:    _width /= 8; _height /= 8; break;
		case BackbufferRatio::Sixteenth: _width /= 16; _height /= 16; break;
		case BackbufferRatio::Double:    _width *= 2; _height *= 2; break;

		default:
			break;
		}

		_width = bx::max<uint16_t>(1, _width);
		_height = bx::max<uint16_t>(1, _height);
	}

	TextureFormat::Enum getViableTextureFormat(const bimg::ImageContainer& _imageContainer)
	{
		const uint32_t formatCaps = g_caps.formats[_imageContainer.m_format];
		bool convert = 0 == formatCaps;

		if (_imageContainer.m_cubeMap)
		{
			convert |= 0 == (formatCaps & MYGFX_CAPS_FORMAT_TEXTURE_CUBE)
				&& 0 != (formatCaps & MYGFX_CAPS_FORMAT_TEXTURE_CUBE_EMULATED)
				;
		}
		else if (_imageContainer.m_depth > 1)
		{
			convert |= 0 == (formatCaps & MYGFX_CAPS_FORMAT_TEXTURE_3D)
				&& 0 != (formatCaps & MYGFX_CAPS_FORMAT_TEXTURE_3D_EMULATED)
				;
		}
		else
		{
			convert |= 0 == (formatCaps & MYGFX_CAPS_FORMAT_TEXTURE_2D)
				&& 0 != (formatCaps & MYGFX_CAPS_FORMAT_TEXTURE_2D_EMULATED)
				;
		}

		if (convert)
		{
			return TextureFormat::BGRA8;
		}

		return TextureFormat::Enum(_imageContainer.m_format);
	}
}