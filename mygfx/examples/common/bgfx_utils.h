#pragma once

#include <bx/pixelformat.h>
#include <mygfx/mygfx.h>
#include <bimg/bimg.h>

///
void* load(const char* _filePath, uint32_t* _size = NULL);

///
void unload(void* _ptr);

///
mygfx::ShaderHandle loadShader(const char* _name);

///
mygfx::ProgramHandle loadProgram(const char* _vsName, const char* _fsName);

///
mygfx::TextureHandle loadTexture(const char* _name, uint32_t _flags = MYGFX_SAMPLER_NONE, uint8_t _skip = 0, mygfx::TextureInfo* _info = NULL, bimg::Orientation::Enum* _orientation = NULL);

///
bimg::ImageContainer* imageLoad(const char* _filePath, mygfx::TextureFormat::Enum _dstFormat);

///
void calcTangents(void* _vertices, uint16_t _numVertices, mygfx::VertexDecl _decl, const uint16_t* _indices, uint32_t _numIndices);

/// Returns true if both internal transient index and vertex buffer have
/// enough space.
///
/// @param[in] _numVertices Number of vertices.
/// @param[in] _decl Vertex declaration.
/// @param[in] _numIndices Number of indices.
///
inline bool checkAvailTransientBuffers(uint32_t _numVertices, const mygfx::VertexDecl& _decl, uint32_t _numIndices)
{
	return _numVertices == mygfx::getAvailTransientVertexBuffer(_numVertices, _decl)
		&& (0 == _numIndices || _numIndices == mygfx::getAvailTransientIndexBuffer(_numIndices) )
		;
}

///
inline uint32_t encodeNormalRgba8(float _x, float _y = 0.0f, float _z = 0.0f, float _w = 0.0f)
{
	const float src[] =
	{
		_x * 0.5f + 0.5f,
		_y * 0.5f + 0.5f,
		_z * 0.5f + 0.5f,
		_w * 0.5f + 0.5f,
	};
	uint32_t dst;
	bx::packRgba8(&dst, src);
	return dst;
}

///
struct MeshState
{
	struct Texture
	{
		uint32_t            m_flags;
		mygfx::UniformHandle m_sampler;
		mygfx::TextureHandle m_texture;
		uint8_t             m_stage;
	};

	Texture             m_textures[4];
	uint64_t            m_state;
	mygfx::ProgramHandle m_program;
	uint8_t             m_numTextures;
	mygfx::ViewId        m_viewId;
};

struct Mesh;

///
Mesh* meshLoad(const char* _filePath);

///
void meshUnload(Mesh* _mesh);

///
MeshState* meshStateCreate();

///
void meshStateDestroy(MeshState* _meshState);

///
void meshSubmit(const Mesh* _mesh, mygfx::ViewId _id, mygfx::ProgramHandle _program, const float* _mtx, uint64_t _state = MYGFX_STATE_MASK);

///
void meshSubmit(const Mesh* _mesh, const MeshState*const* _state, uint8_t _numPasses, const float* _mtx, uint16_t _numMatrices = 1);

///
struct Args
{
	Args(int _argc, const char* const* _argv);

	mygfx::RendererType::Enum m_type;
	uint16_t m_pciId;
};

