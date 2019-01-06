#pragma once

#include "Base.h"

namespace mygfx
{
	struct TextVideoMem
	{
		TextVideoMem();

		~TextVideoMem();

		void resize(bool _small, uint32_t _width, uint32_t _height);

		void clear(uint8_t _attr = 0);

		void printfVargs(uint16_t _x, uint16_t _y, uint8_t _attr, const char* _format, va_list _argList);

		void printf(uint16_t _x, uint16_t _y, uint8_t _attr, const char* _format, ...);

		void image(uint16_t _x, uint16_t _y, uint16_t _width, uint16_t _height, const void* _data, uint16_t _pitch);

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
}