#ifndef MYGFX_H_HEADER_GUARD
#define MYGFX_H_HEADER_GUARD

#include <stdarg.h> // va_list
#include <stdint.h> // uint32_t
#include <stdlib.h> // NULL

#include "defines.h"

#define MYGFX_HANDLE(_name)                                                           \
	struct _name { uint16_t idx; };                                                  \
	inline bool isValid(_name _handle) { return mygfx::kInvalidHandle != _handle.idx; }

#define MYGFX_INVALID_HANDLE { mygfx::kInvalidHandle }

namespace mygfx
{
	struct Fatal
	{
		enum Enum
		{
			DebugCheck,
			InvalidShader,
			UnableToInitialize,
			UnableToCreateTexture,
			DeviceLost,

			Count
		};
	};

	struct RendererType
	{
		/// Renderer types:
		enum Enum
		{
			Noop,         //!< No rendering.
			Direct3D9,    //!< Direct3D 9.0
			Direct3D11,   //!< Direct3D 11.0
			Direct3D12,   //!< Direct3D 12.0
			Metal,        //!< Metal
			OpenGLES,     //!< OpenGL ES 2.0+
			OpenGL,       //!< OpenGL 2.1+
			Vulkan,       //!< Vulkan

			Count
		};
	};

	struct Access
	{
		/// Access:
		enum Enum
		{
			Read,      //!< Read
			Write,     //!< Write
			ReadWrite, //!< Read and write

			Count
		};
	};

	struct Attrib
	{
		/// Corresponds to vertex shader attribute.
		enum Enum
		{
			Position,  //!< a_position
			Normal,    //!< a_normal
			Tangent,   //!< a_tangent
			Bitangent, //!< a_bitangent
			Color0,    //!< a_color0
			Color1,    //!< a_color1
			Color2,    //!< a_color2
			Color3,    //!< a_color3
			Indices,   //!< a_indices
			Weight,    //!< a_weight
			TexCoord0, //!< a_texcoord0
			TexCoord1, //!< a_texcoord1
			TexCoord2, //!< a_texcoord2
			TexCoord3, //!< a_texcoord3
			TexCoord4, //!< a_texcoord4
			TexCoord5, //!< a_texcoord5
			TexCoord6, //!< a_texcoord6
			TexCoord7, //!< a_texcoord7

			Count
		};
	};

	struct AttribType
	{
		/// Attribute types:
		enum Enum
		{
			Uint8,  //!< Uint8
			Uint10, //!< Uint10, availability depends on: `BGFX_CAPS_VERTEX_ATTRIB_UINT10`.
			Int16,  //!< Int16
			Half,   //!< Half, availability depends on: `BGFX_CAPS_VERTEX_ATTRIB_HALF`.
			Float,  //!< Float

			Count
		};
	};

	struct TextureFormat
	{
		/// Texture formats:
		enum Enum
		{
			BC1,          //!< DXT1
			BC2,          //!< DXT3
			BC3,          //!< DXT5
			BC4,          //!< LATC1/ATI1
			BC5,          //!< LATC2/ATI2
			BC6H,         //!< BC6H
			BC7,          //!< BC7
			ETC1,         //!< ETC1 RGB8
			ETC2,         //!< ETC2 RGB8
			ETC2A,        //!< ETC2 RGBA8
			ETC2A1,       //!< ETC2 RGB8A1
			PTC12,        //!< PVRTC1 RGB 2BPP
			PTC14,        //!< PVRTC1 RGB 4BPP
			PTC12A,       //!< PVRTC1 RGBA 2BPP
			PTC14A,       //!< PVRTC1 RGBA 4BPP
			PTC22,        //!< PVRTC2 RGBA 2BPP
			PTC24,        //!< PVRTC2 RGBA 4BPP
			ATC,          //!< ATC RGB 4BPP
			ATCE,         //!< ATCE RGBA 8 BPP explicit alpha
			ATCI,         //!< ATCI RGBA 8 BPP interpolated alpha
			ASTC4x4,      //!< ASTC 4x4 8.0 BPP
			ASTC5x5,      //!< ASTC 5x5 5.12 BPP
			ASTC6x6,      //!< ASTC 6x6 3.56 BPP
			ASTC8x5,      //!< ASTC 8x5 3.20 BPP
			ASTC8x6,      //!< ASTC 8x6 2.67 BPP
			ASTC10x5,     //!< ASTC 10x5 2.56 BPP

			Unknown,      // Compressed formats above.

			R1,
			A8,
			R8,
			R8I,
			R8U,
			R8S,
			R16,
			R16I,
			R16U,
			R16F,
			R16S,
			R32I,
			R32U,
			R32F,
			RG8,
			RG8I,
			RG8U,
			RG8S,
			RG16,
			RG16I,
			RG16U,
			RG16F,
			RG16S,
			RG32I,
			RG32U,
			RG32F,
			RGB8,
			RGB8I,
			RGB8U,
			RGB8S,
			RGB9E5F,
			BGRA8,
			RGBA8,
			RGBA8I,
			RGBA8U,
			RGBA8S,
			RGBA16,
			RGBA16I,
			RGBA16U,
			RGBA16F,
			RGBA16S,
			RGBA32I,
			RGBA32U,
			RGBA32F,
			R5G6B5,
			RGBA4,
			RGB5A1,
			RGB10A2,
			RG11B10F,

			UnknownDepth, // Depth formats below.

			D16,
			D24,
			D24S8,
			D32,
			D16F,
			D24F,
			D32F,
			D0S8,

			Count
		};
	};

	struct UniformType
	{
		/// Uniform types:
		enum Enum
		{
			Int1, //!< Int, used for samplers only.
			End,  //!< Reserved, do not use.

			Vec4, //!< 4 floats vector.
			Mat3, //!< 3x3 matrix.
			Mat4, //!< 4x4 matrix.

			Count
		};
	};

	struct BackbufferRatio
	{
		/// Backbuffer ratios:
		enum Enum
		{
			Equal,     //!< Equal to backbuffer.
			Half,      //!< One half size of backbuffer.
			Quarter,   //!< One quarter size of backbuffer.
			Eighth,    //!< One eighth size of backbuffer.
			Sixteenth, //!< One sixteenth size of backbuffer.
			Double,    //!< Double size of backbuffer.

			Count
		};
	};

	struct OcclusionQueryResult
	{
		/// Occlusion query results:
		enum Enum
		{
			Invisible, //!< Query failed test.
			Visible,   //!< Query passed test.
			NoResult,  //!< Query result is not available yet.

			Count
		};
	};

	struct Topology
	{
		/// Primitive topology:
		enum Enum
		{
			TriList,   //!< Triangle list.
			TriStrip,  //!< Triangle strip.
			LineList,  //!< Line list.
			LineStrip, //!< Line strip.
			PointList, //!< Point list.

			Count
		};
	};

	struct TopologyConvert
	{
		enum Enum
		{
			TriListFlipWinding,  //!< Flip winding order of triangle list.
			TriStripFlipWinding, //!< Flip winding order of trinagle strip.
			TriListToLineList,   //!< Convert triangle list to line list.
			TriStripToTriList,   //!< Convert triangle strip to triangle list.
			LineStripToLineList, //!< Convert line strip to line list.

			Count
		};
	};

	struct TopologySort
	{
		enum Enum
		{
			DirectionFrontToBackMin, //!<
			DirectionFrontToBackAvg, //!<
			DirectionFrontToBackMax, //!<
			DirectionBackToFrontMin, //!<
			DirectionBackToFrontAvg, //!<
			DirectionBackToFrontMax, //!<
			DistanceFrontToBackMin,  //!<
			DistanceFrontToBackAvg,  //!<
			DistanceFrontToBackMax,  //!<
			DistanceBackToFrontMin,  //!<
			DistanceBackToFrontAvg,  //!<
			DistanceBackToFrontMax,  //!<

			Count
		};
	};

	struct ViewMode
	{
		/// View modes:
		enum Enum
		{
			Default,         //!< Default sort order.
			Sequential,      //!< Sort in the same order in which submit calls were called.
			DepthAscending,  //!< Sort draw call depth in ascending order.
			DepthDescending, //!< Sort draw call depth in descending order.

			Count
		};
	};

	static const uint16_t kInvalidHandle = UINT16_MAX;

	MYGFX_HANDLE(DynamicIndexBufferHandle)
		MYGFX_HANDLE(DynamicVertexBufferHandle)
		MYGFX_HANDLE(FrameBufferHandle)
		MYGFX_HANDLE(IndexBufferHandle)
		//MYGFX_HANDLE(IndirectBufferHandle)
		MYGFX_HANDLE(OcclusionQueryHandle)
		MYGFX_HANDLE(ProgramHandle)
		MYGFX_HANDLE(ShaderHandle)
		MYGFX_HANDLE(TextureHandle)
		MYGFX_HANDLE(UniformHandle)
		MYGFX_HANDLE(VertexBufferHandle)
		MYGFX_HANDLE(VertexDeclHandle)

	struct CallbackI
	{
		virtual ~CallbackI() = 0;

		virtual void fatal(
			const char* _filePath
			, uint16_t _line
			, Fatal::Enum _code
			, const char* _str
		) = 0;

		virtual void traceVargs(
			const char* _filePath
			, uint16_t _line
			, const char* _format
			, va_list _argList
		) = 0;
	};

	struct PlatformData
	{
		PlatformData();

		void* ndt;          //!< Native display type.
		void* nwh;          //!< Native window handle.
		void* context;      //!< GL context, or D3D device.
		void* backBuffer;   //!< GL backbuffer, or D3D render target view.
		void* backBufferDS; //!< Backbuffer depth/stencil.
	};

	struct Resolution
	{
		Resolution();

		TextureFormat::Enum format; //!< Backbuffer format.
		uint32_t width;             //!< Backbuffer width.
		uint32_t height;            //!< Backbuffer height.
		uint32_t reset;	            //!< Reset parameters.
		uint8_t  numBackBuffers;    //!< Number of back buffers.
		uint8_t  maxFrameLatency;   //!< Maximum frame latency.
	};

	struct Init
	{
		Init();

		RendererType::Enum type;
		uint16_t vendorId;
		uint16_t deviceId;

		bool debug;   //!< Enable device for debuging.
		bool profile; //!< Enable device for profiling.

		PlatformData platformData;

		Resolution resolution;

		struct Limits
		{
			uint16_t maxEncoders;     //!< Maximum number of encoder threads.
			uint32_t transientVbSize; //!< Maximum transient vertex buffer size.
			uint32_t transientIbSize; //!< Maximum transient index buffer size.
		};

		Limits limits;

		CallbackI* callback;
	};

	typedef void(*ReleaseFn)(void* _ptr, void* _userData);

	struct Memory
	{
		uint8_t* data; //!< Pointer to data.
		uint32_t size; //!< Data size.
	};

	struct Caps
	{
		RendererType rendererType;

		uint64_t supported;

		uint16_t vendorId;         //!< Selected GPU vendor PCI id.
		uint16_t deviceId;         //!< Selected GPU device id.
		bool     homogeneousDepth; //!< True when NDC depth is in [-1, 1] range, otherwise its [0, 1].
		bool     originBottomLeft; //!< True when NDC origin is at bottom left.
		uint8_t  numGPUs;          //!< Number of enumerated GPUs.

		struct GPU
		{
			uint16_t vendorId; //!< Vendor PCI id. See `BGFX_PCI_ID_*`.
			uint16_t deviceId; //!< Device id.
		};

		GPU gpu[4]; //!< Enumerated GPUs.

		struct Limits
		{
			uint32_t maxDrawCalls;            //!< Maximum number of draw calls.
			uint32_t maxBlits;                //!< Maximum number of blit calls.
			uint32_t maxTextureSize;          //!< Maximum texture size.
			uint32_t maxTextureLayers;        //!< Maximum texture layers.
			uint32_t maxViews;                //!< Maximum number of views.
			uint32_t maxFrameBuffers;         //!< Maximum number of frame buffer handles.
			uint32_t maxFBAttachments;        //!< Maximum number of frame buffer attachments.
			uint32_t maxPrograms;             //!< Maximum number of program handles.
			uint32_t maxShaders;              //!< Maximum number of shader handles.
			uint32_t maxTextures;             //!< Maximum number of texture handles.
			uint32_t maxTextureSamplers;      //!< Maximum number of texture samplers.
			uint32_t maxComputeBindings;      //!< Maximum number of compute bindings.
			uint32_t maxVertexDecls;          //!< Maximum number of vertex format declarations.
			uint32_t maxVertexStreams;        //!< Maximum number of vertex streams.
			uint32_t maxIndexBuffers;         //!< Maximum number of index buffer handles.
			uint32_t maxVertexBuffers;        //!< Maximum number of vertex buffer handles.
			uint32_t maxDynamicIndexBuffers;  //!< Maximum number of dynamic index buffer handles.
			uint32_t maxDynamicVertexBuffers; //!< Maximum number of dynamic vertex buffer handles.
			uint32_t maxUniforms;             //!< Maximum number of uniform handles.
			uint32_t maxOcclusionQueries;     //!< Maximum number of occlusion query handles.
			uint32_t maxEncoders;             //!< Maximum number of encoder threads.
			uint32_t transientVbSize;         //!< Maximum transient vertex buffer size.
			uint32_t transientIbSize;         //!< Maximum transient index buffer size.
		};

		Limits limits;

		uint16_t formats[(int32_t)TextureFormat::Count];
	};

	struct TransientIndexBuffer
	{
		uint8_t* data;            //!< Pointer to data.
		uint32_t size;            //!< Data size.
		uint32_t startIndex;      //!< First index.
		IndexBufferHandle handle; //!< Index buffer handle.
	};

	struct TransientVertexBuffer
	{
		uint8_t* data;             //!< Pointer to data.
		uint32_t size;             //!< Data size.
		uint32_t startVertex;      //!< First vertex.
		uint16_t stride;           //!< Vertex stride.
		VertexBufferHandle handle; //!< Vertex buffer handle.
		VertexDeclHandle decl;     //!< Vertex declaration handle.
	};

	/*
	struct InstanceDataBuffer
	{
		uint8_t* data;             //!< Pointer to data.
		uint32_t size;             //!< Data size.
		uint32_t offset;           //!< Offset in vertex buffer.
		uint32_t num;              //!< Number of instances.
		uint16_t stride;           //!< Vertex buffer stride.
		VertexBufferHandle handle; //!< Vertex buffer object handle.
	};
*/

	struct TextureInfo
	{
		TextureFormat::Enum format; //!< Texture format.
		uint32_t storageSize;       //!< Total amount of bytes required to store texture.
		uint16_t width;             //!< Texture width.
		uint16_t height;            //!< Texture height.
		uint16_t depth;             //!< Texture depth.
		uint16_t numLayers;         //!< Number of layers in texture array.
		uint8_t numMips;            //!< Number of MIP maps.
		uint8_t bitsPerPixel;       //!< Format bits per pixel.
		bool    cubeMap;            //!< Texture is cubemap.
	};

	struct UniformInfo
	{
		char name[256];         //!< Uniform name.
		UniformType::Enum type; //!< Uniform type.
		uint16_t num;           //!< Number of elements in array.
	};

	struct Attachment
	{
		TextureHandle handle; //!< Texture handle.
		uint16_t mip;         //!< Mip level.
		uint16_t layer;       //!< Cubemap side or depth layer/slice.
		uint8_t  resolve;     //!< Resolve flags. See: `BGFX_RESOLVE_*`
	};

	struct Transform
	{
		float* data;  //!< Pointer to first 4x4 matrix.
		uint16_t num; //!< Number of matrices.
	};

	typedef uint16_t ViewId;

	struct ViewStats
	{
		char    name[256];      //!< View name.
		ViewId  view;           //!< View id.
		int64_t cpuTimeElapsed; //!< CPU (submit) time elapsed.
		int64_t gpuTimeElapsed; //!< GPU time elapsed.
	};

	struct EncoderStats
	{
		int64_t cpuTimeBegin; //!< Encoder thread CPU submit begin time.
		int64_t cpuTimeEnd;   //!< Encoder thread CPU submit end time.
	};

	struct Stats
	{
		int64_t cpuTimeFrame;               //!< CPU time between two `bgfx::frame` calls.
		int64_t cpuTimeBegin;               //!< Render thread CPU submit begin time.
		int64_t cpuTimeEnd;                 //!< Render thread CPU submit end time.
		int64_t cpuTimerFreq;               //!< CPU timer frequency. Timestamps-per-second

		int64_t gpuTimeBegin;               //!< GPU frame begin time.
		int64_t gpuTimeEnd;                 //!< GPU frame end time.
		int64_t gpuTimerFreq;               //!< GPU timer frequency.

		int64_t waitRender;                 //!< Time spent waiting for render backend thread to finish issuing
											//!  draw commands to underlying graphics API.
		int64_t waitSubmit;                 //!< Time spent waiting for submit thread to advance to next frame.

		uint32_t numDraw;                   //!< Number of draw calls submitted.
		uint32_t numCompute;                //!< Number of compute calls submitted.
		uint32_t numBlit;                   //!< Number of blit calls submitted.
		uint32_t maxGpuLatency;             //!< GPU driver latency.

		uint16_t numDynamicIndexBuffers;    //!< Number of used dynamic index buffers.
		uint16_t numDynamicVertexBuffers;   //!< Number of used dynamic vertex buffers.
		uint16_t numFrameBuffers;           //!< Number of used frame buffers.
		uint16_t numIndexBuffers;           //!< Number of used index buffers.
		uint16_t numOcclusionQueries;       //!< Number of used occlusion queries.
		uint16_t numPrograms;               //!< Number of used programs.
		uint16_t numShaders;                //!< Number of used shaders.
		uint16_t numTextures;               //!< Number of used textures.
		uint16_t numUniforms;               //!< Number of used uniforms.
		uint16_t numVertexBuffers;          //!< Number of used vertex buffers.
		uint16_t numVertexDecls;            //!< Number of used vertex declarations.

		int64_t textureMemoryUsed;          //!< Estimate of texture memory used.
		int64_t rtMemoryUsed;               //!< Estimate of render target memory used.
		int32_t transientVbUsed;            //!< Amount of transient vertex buffer used.
		int32_t transientIbUsed;            //!< Amount of transient index buffer used.

		uint32_t numPrims[(int32_t)Topology::Count]; //!< Number of primitives rendered.

		int64_t gpuMemoryMax;               //!< Maximum available GPU memory for application.
		int64_t gpuMemoryUsed;              //!< Amount of GPU memory used by the application.

		uint16_t width;                     //!< Backbuffer width in pixels.
		uint16_t height;                    //!< Backbuffer height in pixels.
		uint16_t textWidth;                 //!< Debug text width in characters.
		uint16_t textHeight;                //!< Debug text height in characters.

		uint16_t   numViews;                //!< Number of view stats.
		ViewStats* viewStats;               //!< Array of View stats.

		uint8_t       numEncoders;          //!< Number of encoders used during frame.
		EncoderStats* encoderStats;         //!< Array of encoder stats.
	};

	/// Encoders are used for submitting draw calls from multiple threads. Only one encoder
	/// per thread should be used. Use `bgfx::begin()` to obtain an encoder for a thread.

	struct Encoder
	{
		void setMarker(const char* _marker);

		void setState(
			uint64_t _state
			, uint32_t _rgba = 0
		);

		void setCondition(
			OcclusionQueryHandle _handle
			, bool _visible
		);

		void setStencil(
			uint32_t _fstencil
			, uint32_t _bstencil = MYGFX_STENCIL_NONE
		);

		uint16_t setScissor(
			uint16_t _x
			, uint16_t _y
			, uint16_t _width
			, uint16_t _height
		);

		void setScissor(uint16_t _cache = UINT16_MAX);

		uint32_t setTransform(
			const void* _mtx
			, uint16_t _num = 1
		);

		uint32_t allocTransform(
			Transform* _transform
			, uint16_t _num
		);

		void setTransform(
			uint32_t _cache
			, uint16_t _num = 1
		);

		void setUniform(
			UniformHandle _handle
			, const void* _value
			, uint16_t _num = 1
		);

		void setIndexBuffer(IndexBufferHandle _handle);

		void setIndexBuffer(
			IndexBufferHandle _handle
			, uint32_t _firstIndex
			, uint32_t _numIndices
		);

		void setIndexBuffer(DynamicIndexBufferHandle _handle);

		void setIndexBuffer(
			DynamicIndexBufferHandle _handle
			, uint32_t _firstIndex
			, uint32_t _numIndices
		);

		void setIndexBuffer(const TransientIndexBuffer* _tib);

		void setIndexBuffer(
			const TransientIndexBuffer* _tib
			, uint32_t _firstIndex
			, uint32_t _numIndices
		);

		void setVertexBuffer(
			uint8_t _stream
			, VertexBufferHandle _handle
		);

		void setVertexBuffer(
			uint8_t _stream
			, VertexBufferHandle _handle
			, uint32_t _startVertex
			, uint32_t _numVertices
		);

		void setVertexBuffer(
			uint8_t _stream
			, DynamicVertexBufferHandle _handle
		);

		void setVertexBuffer(
			uint8_t _stream
			, DynamicVertexBufferHandle _handle
			, uint32_t _startVertex
			, uint32_t _numVertices
		);

		void setVertexBuffer(
			uint8_t _stream
			, const TransientVertexBuffer* _tvb
		);

		void setVertexBuffer(
			uint8_t _stream
			, const TransientVertexBuffer* _tvb
			, uint32_t _startVertex
			, uint32_t _numVertices
		);

		void setTexture(
			uint8_t _stage
			, UniformHandle _sampler
			, TextureHandle _handle
			, uint32_t _flags = UINT32_MAX
		);

		/// Submit an empty primitive for rendering. Uniforms and draw state
		/// will be applied but no geometry will be submitted.
		void touch(ViewId _id);

		void submit(
			ViewId _id
			, ProgramHandle _program
			, uint32_t _depth = 0
			, bool _preserveState = false
		);

		void submit(
			ViewId _id
			, ProgramHandle _program
			, OcclusionQueryHandle _occlusionQuery
			, uint32_t _depth = 0
			, bool _preserveState = false
		);

		/// Set compute index buffer.
		void setBuffer(
			uint8_t _stage
			, IndexBufferHandle _handle
			, Access _access
		);

		void setBuffer(
			uint8_t _stage
			, VertexBufferHandle _handle
			, Access _access
		);

		void setBuffer(
			uint8_t _stage
			, DynamicIndexBufferHandle _handle
			, Access _access
		);

		void setBuffer(
			uint8_t _stage
			, DynamicVertexBufferHandle _handle
			, Access _access
		);

		void setImage(
			uint8_t _stage
			, TextureHandle _handle
			, uint8_t _mip
			, Access _access
			, TextureFormat::Enum _format = TextureFormat::Enum::Count
		);

		/// Dispatch compute.
		void dispatch(
			ViewId _id
			, ProgramHandle _handle
			, uint32_t _numX = 1
			, uint32_t _numY = 1
			, uint32_t _numZ = 1
			, uint8_t _flags = MYGFX_SUBMIT_EYE_FIRST
		);

		/// Discard all previously set state for draw or compute call.
		void discard();

		void blit(
			ViewId _id
			, TextureHandle _dst
			, uint16_t _dstX
			, uint16_t _dstY
			, TextureHandle _src
			, uint16_t _srcX = 0
			, uint16_t _srcY = 0
			, uint16_t _width = UINT16_MAX
			, uint16_t _height = UINT16_MAX
		);

		void blit(
			ViewId _id
			, TextureHandle _dst
			, uint8_t _dstMip
			, uint16_t _dstX
			, uint16_t _dstY
			, uint16_t _dstZ
			, TextureHandle _src
			, uint8_t _srcMip = 0
			, uint16_t _srcX = 0
			, uint16_t _srcY = 0
			, uint16_t _srcZ = 0
			, uint16_t _width = UINT16_MAX
			, uint16_t _height = UINT16_MAX
			, uint16_t _depth = UINT16_MAX
		);
	};

	struct VertexDecl 
	{
		VertexDecl();

		VertexDecl& begin(RendererType::Enum _renderer = RendererType::Noop);

		void end();

		VertexDecl& add(
			Attrib::Enum _attrib
			, uint8_t _num
			, AttribType::Enum _type
			, bool _normalized = false
			, bool _asInt = false
		);

		VertexDecl& skip(uint8_t _num);

		void decode(
			Attrib::Enum _attrib
			, uint8_t& _num
			, AttribType::Enum& _type
			, bool& _normalized
			, bool& _asInt
		) const;

		bool has(Attrib::Enum _attrib) const { return UINT16_MAX != m_attributes[_attrib]; }

		/// Returns relative attribute offset from the vertex.
		uint16_t getOffset(Attrib::Enum _attrib) const { return m_offset[_attrib]; }

		/// Returns vertex stride.
		uint16_t getStride() const { return m_stride; }

		/// Returns size of vertex buffer for number of vertices.
		uint32_t getSize(uint32_t _num) const { return _num*m_stride; }

		uint32_t m_hash;
		uint16_t m_stride;
		uint16_t m_offset[Attrib::Count];
		uint16_t m_attributes[Attrib::Count];
	};

}


namespace mygfx
{
	void vertexPack(
		const float _input[4]
		, bool _inputNormalized
		, Attrib::Enum _attr
		, const VertexDecl& _decl
		, void* _data
		, uint32_t _index = 0
	);

	void vertexUnpack(
		float _output[4]
		, Attrib::Enum _attr
		, const VertexDecl& _decl
		, const void* _data
		, uint32_t _index = 0
	);

	void vertexConvert(
		const VertexDecl& _destDecl
		, void* _destData
		, const VertexDecl& _srcDecl
		, const void* _srcData
		, uint32_t _num = 1
	);

	uint16_t weldVertices(
		uint16_t* _output
		, const VertexDecl& _decl
		, const void* _data
		, uint16_t _num
		, float _epsilon = 0.001f
	);

	uint32_t topologyConvert(
		TopologyConvert::Enum _conversion
		, void* _dst
		, uint32_t _dstSize
		, const void* _indices
		, uint32_t _numIndices
		, bool _index32
	);

	void topologySortTriList(
		TopologySort::Enum _sort
		, void* _dst
		, uint32_t _dstSize
		, const float _dir[3]
		, const float _pos[3]
		, const void* _vertices
		, uint32_t _stride
		, const void* _indices
		, uint32_t _numIndices
		, bool _index32
	);

	uint8_t getSupportedRenderers(
		uint8_t _max = 0
		, RendererType::Enum* _enum = NULL
	);

	const char* getRendererName(RendererType::Enum _type);

	//
	bool init(const Init& _init = {});

	void shutdown();

	void reset(
		uint32_t _width
		, uint32_t _height
		, uint32_t _flags = MYGFX_RESET_NONE
		, TextureFormat::Enum _format = TextureFormat::Count
	);

	Encoder* begin(bool _forThread = false);

	void end(Encoder* _encoder);

	uint32_t frame(bool _capture = false);

	RendererType::Enum getRendererType();

	const Caps* getCaps();

	const Stats* getStats();

	const Memory* alloc(uint32_t _size);

	const Memory* copy(
		const void* _data
		, uint32_t _size
	);

	const Memory* makeRef(
		const void* _data
		, uint32_t _size
		, ReleaseFn _releaseFn = NULL
		, void* _userData = NULL
	);

	void setDebug(uint32_t _debug);

	void dbgTextClear(
		uint8_t _attr = 0
		, bool _small = false
	);

	void dbgTextPrintf(
		uint16_t _x
		, uint16_t _y
		, uint8_t _attr
		, const char* _format
		, ...
	);

	void dbgTextPrintfVargs(
		uint16_t _x
		, uint16_t _y
		, uint8_t _attr
		, const char* _format
		, va_list _argList
	);

	void dbgTextImage(
		uint16_t _x
		, uint16_t _y
		, uint16_t _width
		, uint16_t _height
		, const void* _data
		, uint16_t _pitch
	);

	//
	IndexBufferHandle createIndexBuffer(
		const Memory* _mem
		, uint16_t _flags = MYGFX_BUFFER_NONE
	);

	void destroy(IndexBufferHandle _handle);

	VertexBufferHandle createVertexBuffer(
		const Memory* _mem
		, const VertexDecl& _decl
		, uint16_t _flags = MYGFX_BUFFER_NONE
	);

	void destroy(VertexBufferHandle _handle);

	DynamicIndexBufferHandle createDynamicIndexBuffer(
		uint32_t _num
		, uint16_t _flags = MYGFX_BUFFER_NONE
	);

	DynamicIndexBufferHandle createDynamicIndexBuffer(
		const Memory* _mem
		, uint16_t _flags = MYGFX_BUFFER_NONE
	);

	void update(
		DynamicIndexBufferHandle _handle
		, uint32_t _startIndex
		, const Memory* _mem
	);

	void destroy(DynamicIndexBufferHandle _handle);

	DynamicVertexBufferHandle createDynamicVertexBuffer(
		uint32_t _num
		, const VertexDecl& _decl
		, uint16_t _flags = MYGFX_BUFFER_NONE
	);

	DynamicVertexBufferHandle createDynamicVertexBuffer(
		const Memory* _mem
		, const VertexDecl& _decl
		, uint16_t _flags = MYGFX_BUFFER_NONE
	);

	void update(
		DynamicVertexBufferHandle _handle
		, uint32_t _startVertex
		, const Memory* _mem
	);

	void destroy(DynamicVertexBufferHandle _handle);

	//
	uint32_t getAvailTransientIndexBuffer(uint32_t _num);

	uint32_t getAvailTransientVertexBuffer(
		uint32_t _num
		, const VertexDecl& _decl
	);

	void allocTransientIndexBuffer(
		TransientIndexBuffer* _tib
		, uint32_t _num
	);

	void allocTransientVertexBuffer(
		TransientVertexBuffer* _tvb
		, uint32_t _num
		, const VertexDecl& _decl
	);

	bool allocTransientBuffers(
		TransientVertexBuffer* _tvb
		, const VertexDecl& _decl
		, uint32_t _numVertices
		, TransientIndexBuffer* _tib
		, uint32_t _numIndices
	);

	//
	ShaderHandle createShader(const Memory* _mem);

	uint16_t getShaderUniforms(
		ShaderHandle _handle
		, UniformHandle* _uniforms = NULL
		, uint16_t _max = 0
	);

	void setName(
		ShaderHandle _handle
		, const char* _name
		, int32_t _len = INT32_MAX
	);

	void destroy(ShaderHandle _handle);

	//
	ProgramHandle createProgram(
		ShaderHandle _vsh
		, ShaderHandle _fsh
		, bool _destroyShaders = false
	);

	ProgramHandle createProgram(
		ShaderHandle _csh
		, bool _destroyShader = false
	);

	void destroy(ProgramHandle _handle);

	bool isTextureValid(
		uint16_t _depth
		, bool _cubeMap
		, uint16_t _numLayers
		, TextureFormat::Enum _format
		, uint64_t _flags
	);

	void calcTextureSize(
		TextureInfo& _info
		, uint16_t _width
		, uint16_t _height
		, uint16_t _depth
		, bool _cubeMap
		, bool _hasMips
		, uint16_t _numLayers
		, TextureFormat::Enum _format
	);

	TextureHandle createTexture(
		const Memory* _mem
		, uint64_t _flags = MYGFX_TEXTURE_NONE | MYGFX_SAMPLER_NONE
		, uint8_t _skip = 0
		, TextureInfo* _info = NULL
	);

	TextureHandle createTexture2D(
		uint16_t _width
		, uint16_t _height
		, bool     _hasMips
		, uint16_t _numLayers
		, TextureFormat::Enum _format
		, uint64_t _flags = MYGFX_TEXTURE_NONE | MYGFX_SAMPLER_NONE
		, const Memory* _mem = NULL
	);

	TextureHandle createTexture2D(
		BackbufferRatio::Enum _ratio
		, bool _hasMips
		, uint16_t _numLayers
		, TextureFormat::Enum _format
		, uint64_t _flags = MYGFX_TEXTURE_NONE | MYGFX_SAMPLER_NONE
	);

	TextureHandle createTexture3D(
		uint16_t _width
		, uint16_t _height
		, uint16_t _depth
		, bool _hasMips
		, TextureFormat::Enum _format
		, uint64_t _flags = MYGFX_TEXTURE_NONE | MYGFX_SAMPLER_NONE
		, const Memory* _mem = NULL
	);

	TextureHandle createTextureCube(
		uint16_t _size
		, bool _hasMips
		, uint16_t _numLayers
		, TextureFormat::Enum _format
		, uint64_t _flags = MYGFX_TEXTURE_NONE | MYGFX_SAMPLER_NONE
		, const Memory* _mem = NULL
	);

	void updateTexture2D(
		TextureHandle _handle
		, uint16_t _layer
		, uint8_t _mip
		, uint16_t _x
		, uint16_t _y
		, uint16_t _width
		, uint16_t _height
		, const Memory* _mem
		, uint16_t _pitch = UINT16_MAX
	);

	void updateTexture3D(
		TextureHandle _handle
		, uint8_t _mip
		, uint16_t _x
		, uint16_t _y
		, uint16_t _z
		, uint16_t _width
		, uint16_t _height
		, uint16_t _depth
		, const Memory* _mem
	);

	void updateTextureCube(
		TextureHandle _handle
		, uint16_t _layer
		, uint8_t _side
		, uint8_t _mip
		, uint16_t _x
		, uint16_t _y
		, uint16_t _width
		, uint16_t _height
		, const Memory* _mem
		, uint16_t _pitch = UINT16_MAX
	);

	uint32_t readTexture(
		TextureHandle _handle
		, void* _data
		, uint8_t _mip = 0
	);

	void setName(
		TextureHandle _handle
		, const char* _name
		, int32_t _len = INT32_MAX
	);

	void* getDirectAccessPtr(TextureHandle _handle);

	void destroy(TextureHandle _handle);

	//
	FrameBufferHandle createFrameBuffer(
		uint16_t _width
		, uint16_t _height
		, TextureFormat::Enum _format
		, uint64_t _textureFlags = MYGFX_SAMPLER_U_CLAMP | MYGFX_SAMPLER_V_CLAMP
	);

	FrameBufferHandle createFrameBuffer(
		BackbufferRatio::Enum _ratio
		, TextureFormat::Enum _format
		, uint64_t _textureFlags = MYGFX_SAMPLER_U_CLAMP | MYGFX_SAMPLER_V_CLAMP
	);

	FrameBufferHandle createFrameBuffer(
		uint8_t _num
		, const TextureHandle* _handles
		, bool _destroyTextures = false
	);

	FrameBufferHandle createFrameBuffer(
		uint8_t _num
		, const Attachment* _attachment
		, bool _destroyTextures = false
	);

	FrameBufferHandle createFrameBuffer(
		void* _nwh
		, uint16_t _width
		, uint16_t _height
		, TextureFormat::Enum _format = TextureFormat::Count
		, TextureFormat::Enum _depthFormat = TextureFormat::Count
	);

	TextureHandle getTexture(
		FrameBufferHandle _handle
		, uint8_t _attachment = 0
	);

	void destroy(FrameBufferHandle _handle);

	UniformHandle createUniform(
		const char* _name
		, UniformType::Enum _type
		, uint16_t _num = 1
	);

	void getUniformInfo(
		UniformHandle _handle
		, UniformInfo& _info
	);

	void destroy(UniformHandle _handle);

	//
	OcclusionQueryHandle createOcclusionQuery();

	OcclusionQueryResult::Enum getResult(
		OcclusionQueryHandle _handle
		, int32_t* _result = NULL
	);

	void destroy(OcclusionQueryHandle _handle);

	void setPaletteColor(
		uint8_t _index
		, uint32_t _rgba
	);

	void setPaletteColor(
		uint8_t _index
		, float _r
		, float _g
		, float _b
		, float _a
	);

	void setPaletteColor(
		uint8_t _index
		, const float _rgba[4]
	);

	void setViewName(
		ViewId _id
		, const char* _name
	);

	void setViewRect(
		ViewId _id
		, uint16_t _x
		, uint16_t _y
		, uint16_t _width
		, uint16_t _height
	);

	void setViewRect(
		ViewId _id
		, uint16_t _x
		, uint16_t _y
		, BackbufferRatio::Enum _ratio
	);

	void setViewScissor(
		ViewId _id
		, uint16_t _x = 0
		, uint16_t _y = 0
		, uint16_t _width = 0
		, uint16_t _height = 0
	);

	void setViewClear(
		ViewId _id
		, uint16_t _flags
		, uint32_t _rgba = 0x000000ff
		, float _depth = 1.0f
		, uint8_t _stencil = 0
	);

	void setViewClear(
		ViewId _id
		, uint16_t _flags
		, float _depth
		, uint8_t _stencil
		, uint8_t _0 = UINT8_MAX
		, uint8_t _1 = UINT8_MAX
		, uint8_t _2 = UINT8_MAX
		, uint8_t _3 = UINT8_MAX
		, uint8_t _4 = UINT8_MAX
		, uint8_t _5 = UINT8_MAX
		, uint8_t _6 = UINT8_MAX
		, uint8_t _7 = UINT8_MAX
	);

	void setViewMode(
		ViewId _id
		, ViewMode::Enum _mode = ViewMode::Default
	);

	void setViewFrameBuffer(
		ViewId _id
		, FrameBufferHandle _handle
	);

	void setViewTransform(
		ViewId _id
		, const void* _view
		, const void* _projL
		, uint8_t _flags = MYGFX_VIEW_STEREO
		, const void* _projR = NULL
	);

	void setViewOrder(
		ViewId _id = 0
		, uint16_t _num = UINT16_MAX
		, const ViewId* _remap = NULL
	);

	void resetView(ViewId _id);

	void setMarker(const char* _marker);

	void setState(
		uint64_t _state
		, uint32_t _rgba = 0
	);

	void setCondition(
		OcclusionQueryHandle _handle
		, bool _visible
	);

	void setStencil(
		uint32_t _fstencil
		, uint32_t _bstencil = MYGFX_STENCIL_NONE
	);

	uint16_t setScissor(
		uint16_t _x
		, uint16_t _y
		, uint16_t _width
		, uint16_t _height
	);

	void setScissor(uint16_t _cache = UINT16_MAX);

	uint32_t setTransform(
		const void* _mtx
		, uint16_t _num = 1
	);

	uint32_t allocTransform(
		Transform* _transform
		, uint16_t _num
	);

	void setTransform(
		uint32_t _cache
		, uint16_t _num = 1
	);

	void setUniform(
		UniformHandle _handle
		, const void* _value
		, uint16_t _num = 1
	);

	void setIndexBuffer(IndexBufferHandle _handle);

	void setIndexBuffer(
		IndexBufferHandle _handle
		, uint32_t _firstIndex
		, uint32_t _numIndices
	);

	void setIndexBuffer(DynamicIndexBufferHandle _handle);

	void setIndexBuffer(
		DynamicIndexBufferHandle _handle
		, uint32_t _firstIndex
		, uint32_t _numIndices
	);

	void setIndexBuffer(const TransientIndexBuffer* _tib);

	void setIndexBuffer(
		const TransientIndexBuffer* _tib
		, uint32_t _firstIndex
		, uint32_t _numIndices
	);

	void setVertexBuffer(
		uint8_t _stream
		, VertexBufferHandle _handle
	);

	void setVertexBuffer(
		uint8_t _stream
		, VertexBufferHandle _handle
		, uint32_t _startVertex
		, uint32_t _numVertices
	);

	void setVertexBuffer(
		uint8_t _stream
		, DynamicVertexBufferHandle _handle
	);

	void setVertexBuffer(
		uint8_t _stream
		, DynamicVertexBufferHandle _handle
		, uint32_t _startVertex
		, uint32_t _numVertices
	);

	void setVertexBuffer(
		uint8_t _stream
		, const TransientVertexBuffer* _tvb
	);

	void setVertexBuffer(
		uint8_t _stream
		, const TransientVertexBuffer* _tvb
		, uint32_t _startVertex
		, uint32_t _numVertices
	);

	void setVertexCount(uint32_t _numVertices);

	//
	void setTexture(
		uint8_t _stage
		, UniformHandle _sampler
		, TextureHandle _handle
		, uint32_t _flags = UINT32_MAX
	);

	void touch(ViewId _id);

	void submit(
		ViewId _id
		, ProgramHandle _program
		, uint32_t _depth = 0
		, bool _preserveState = false
	);

	void submit(
		ViewId _id
		, ProgramHandle _program
		, OcclusionQueryHandle _occlusionQuery
		, uint32_t _depth = 0
		, bool _preserveState = false
	);

	void setBuffer(
		uint8_t _stage
		, IndexBufferHandle _handle
		, Access::Enum _access
	);

	void setBuffer(
		uint8_t _stage
		, VertexBufferHandle _handle
		, Access::Enum _access
	);

	void setBuffer(
		uint8_t _stage
		, DynamicIndexBufferHandle _handle
		, Access::Enum _access
	);

	void setBuffer(
		uint8_t _stage
		, DynamicVertexBufferHandle _handle
		, Access::Enum _access
	);

	void setImage(
		uint8_t _stage
		, TextureHandle _handle
		, uint8_t _mip
		, Access::Enum _access
		, TextureFormat::Enum _format = TextureFormat::Count
	);

	void dispatch(
		ViewId _id
		, ProgramHandle _handle
		, uint32_t _numX = 1
		, uint32_t _numY = 1
		, uint32_t _numZ = 1
		, uint8_t _flags = MYGFX_SUBMIT_EYE_FIRST
	);

	void discard();

	void blit(
		ViewId _id
		, TextureHandle _dst
		, uint16_t _dstX
		, uint16_t _dstY
		, TextureHandle _src
		, uint16_t _srcX = 0
		, uint16_t _srcY = 0
		, uint16_t _width = UINT16_MAX
		, uint16_t _height = UINT16_MAX
	);

	void blit(
		ViewId _id
		, TextureHandle _dst
		, uint8_t _dstMip
		, uint16_t _dstX
		, uint16_t _dstY
		, uint16_t _dstZ
		, TextureHandle _src
		, uint8_t _srcMip = 0
		, uint16_t _srcX = 0
		, uint16_t _srcY = 0
		, uint16_t _srcZ = 0
		, uint16_t _width = UINT16_MAX
		, uint16_t _height = UINT16_MAX
		, uint16_t _depth = UINT16_MAX
	);

	void requestScreenShot(
		FrameBufferHandle _handle
		, const char* _filePath
	);
}

#endif
