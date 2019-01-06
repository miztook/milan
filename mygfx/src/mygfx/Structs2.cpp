#include "mygfx_p.h"

namespace mygfx
{
	static uint8_t parseAttrTo(char*& _ptr, char _to, uint8_t _default)
	{
		const bx::StringView str = bx::strFind(_ptr, _to);
		if (!str.isEmpty()
			&& 3 > str.getPtr() - _ptr)
		{
			char tmp[4];

			int32_t len = int32_t(str.getPtr() - _ptr);
			bx::strCopy(tmp, sizeof(tmp), _ptr, len);

			uint32_t attr;
			bx::fromString(&attr, tmp);

			_ptr += len + 1;
			return uint8_t(attr);
		}

		return _default;
	}

	static uint8_t parseAttr(char*& _ptr, uint8_t _default)
	{
		char* ptr = _ptr;
		if (*ptr++ != '[')
		{
			return _default;
		}

		if (0 == bx::strCmp(ptr, "0m", 2))
		{
			_ptr = ptr + 2;
			return _default;
		}

		uint8_t fg = parseAttrTo(ptr, ';', _default & 0xf);
		uint8_t bg = parseAttrTo(ptr, 'm', _default >> 4);

		uint8_t attr = (bg << 4) | fg;
		_ptr = ptr;
		return attr;
	}


	TextVideoMem::TextVideoMem() : m_mem(NULL)
		, m_size(0)
		, m_width(0)
		, m_height(0)
		, m_small(false)
	{
		resize(false, 1, 1);
		clear();
	}

	TextVideoMem::~TextVideoMem()
	{
		BX_FREE(g_allocator, m_mem);
	}

	void TextVideoMem::resize(bool _small, uint32_t _width, uint32_t _height)
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

	void TextVideoMem::clear(uint8_t _attr /*= 0*/)
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

	void TextVideoMem::printfVargs(uint16_t _x, uint16_t _y, uint8_t _attr, const char* _format, va_list _argList)
	{
		if (_x < m_width && _y < m_height)
		{
			va_list argListCopy;
			va_copy(argListCopy, _argList);
			uint32_t num = bx::vsnprintf(NULL, 0, _format, argListCopy) + 1;
			char* temp = (char*)alloca(num);
			va_copy(argListCopy, _argList);
			num = bx::vsnprintf(temp, num, _format, argListCopy);

			uint8_t attr = _attr;
			MemSlot* mem = &m_mem[_y*m_width + _x];
			for (uint32_t ii = 0, xx = _x; ii < num && xx < m_width; ++ii)
			{
				char ch = temp[ii];
				if (BX_UNLIKELY(ch == '\x1b'))
				{
					char* ptr = &temp[ii + 1];
					attr = parseAttr(ptr, _attr);
					ii += uint32_t(ptr - &temp[ii + 1]);
				}
				else
				{
					mem->character = ch;
					mem->attribute = attr;
					++mem;
					++xx;
				}
			}
		}
	}

	void TextVideoMem::printf(uint16_t _x, uint16_t _y, uint8_t _attr, const char* _format, ...)
	{
		va_list argList;
		va_start(argList, _format);
		printfVargs(_x, _y, _attr, _format, argList);
		va_end(argList);
	}

	void TextVideoMem::image(uint16_t _x, uint16_t _y, uint16_t _width, uint16_t _height, const void* _data, uint16_t _pitch)
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

	void TextVideoMemBlitter::init()
	{
		//TODO
	}

	void TextVideoMemBlitter::shutdown()
	{
		//TODO
	}

	void ClearQuad::init()
	{
		//TODO
	}

	void ClearQuad::shutdown()
	{
		//TODO
	}

}