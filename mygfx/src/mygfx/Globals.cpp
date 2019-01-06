#include "Globals.h"
#include "mygfx_p.h"

namespace mygfx
{
	InternalData g_internalData;
	PlatformData g_platformData;
	bool g_platformDataChangedSinceReset = false;

	CallbackI* g_callback = NULL;
	bx::AllocatorI* g_allocator = NULL;
	Caps g_caps;
	const uint32_t g_uniformTypeSize[UniformType::Count + 1] =
	{
		sizeof(int32_t),
		0,
		4 * sizeof(float),
		3 * 3 * sizeof(float),
		4 * 4 * sizeof(float),
		1,
	};

}