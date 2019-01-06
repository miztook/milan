#include "mygfx/platform.h"

#include "Globals.h"
#include "mygfx_p.h"

namespace mygfx
{
	RenderFrame::Enum renderFrame(int32_t _msecs)
	{
		if (BX_ENABLED(MYGFX_CONFIG_MULTITHREADED))
		{
			//TODO
		}

		BX_CHECK(false, "This call only makes sense if used with multi-threaded renderer.");
		return RenderFrame::NoContext;
	}

	void setPlatformData(const PlatformData& _data)
	{
		if (NULL != s_ctx)
		{
			MYGFX_FATAL(true
				&& g_platformData.ndt == _data.ndt
				&& g_platformData.context == _data.context
				, Fatal::UnableToInitialize
				, "Only backbuffer pointer and native window handle can be changed after initialization!"
			);
		}
		bx::memCopy(&g_platformData, &_data, sizeof(PlatformData));
		g_platformDataChangedSinceReset = true;
	}

	const InternalData* getInternalData()
	{
		MYGFX_CHECK_RENDER_THREAD();
		return &g_internalData;
	}
}