#include "mygfx_p.h"

namespace mygfx
{
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

}