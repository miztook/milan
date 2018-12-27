#ifndef MYGFX_SHADER_H
#define MYGFX_SHADER_H

#include <bx/readerwriter.h>

namespace mygfx
{
	///
	void disassemble(bx::WriterI* _writer, bx::ReaderSeekerI* _reader, bx::Error* _err = NULL);

	///
	void disassemble(bx::WriterI* _writer, const void* _data, uint32_t _size, bx::Error* _err = NULL);

} 

#endif
