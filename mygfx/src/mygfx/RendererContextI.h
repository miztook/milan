#pragma once

#include "Structs.h"
#include "Structs2.h"

namespace mygfx
{
	struct Frame;
	class UniformBuffer;

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

	extern void blit(RendererContextI* _renderCtx, TextVideoMemBlitter& _blitter, const TextVideoMem& _mem);

	inline void blit(RendererContextI* _renderCtx, TextVideoMemBlitter& _blitter, const TextVideoMem* _mem)
	{
		blit(_renderCtx, _blitter, *_mem);
	}

	void rendererUpdateUniforms(RendererContextI* _renderCtx, UniformBuffer* _uniformBuffer, uint32_t _begin, uint32_t _end);
};
