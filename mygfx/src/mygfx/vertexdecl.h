
#ifndef MYGFX_VERTEXDECL_H_HEADER_GUARD
#define MYGFX_VERTEXDECL_H_HEADER_GUARD

#include "mygfx/mygfx.h"
#include <bx/readerwriter.h>

namespace mygfx
{
	///
	void initAttribTypeSizeTable(RendererType::Enum _type);

	/// Returns attribute name.
	const char* getAttribName(Attrib::Enum _attr);

	///
	const char* getAttribNameShort(Attrib::Enum _attr);

	/// Dump vertex declaration into debug output.
	void dump(const VertexDecl& _decl);

	///
	Attrib::Enum idToAttrib(uint16_t id);

	///
	uint16_t attribToId(Attrib::Enum _attr);

	///
	AttribType::Enum idToAttribType(uint16_t id);

	///
	int32_t write(bx::WriterI* _writer, const VertexDecl& _decl, bx::Error* _err = NULL);

	///
	int32_t read(bx::ReaderI* _reader, VertexDecl& _decl, bx::Error* _err = NULL);

} // namespace bgfx

#endif