#ifndef MYGFX_PLATFORM_H_HEADER_GUARD
#define MYGFX_PLATFORM_H_HEADER_GUARD

#include <bx/platform.h>
#include "mygfx/mygfx.h"

namespace mygfx
{
	struct RenderFrame
	{
		enum Enum
		{
			NoContext,
			Render,
			Timeout,
			Exiting,

			Count
		};
	};

		/// Render frame.
		///
		/// @param _msecs Timeout in milliseconds.
		///
		/// @returns Current renderer state. See: `bgfx::RenderFrame`.
		///
		/// @attention `bgfx::renderFrame` is blocking call. It waits for
		///   `bgfx::frame` to be called from API thread to process frame.
		///   If timeout value is passed call will timeout and return even
		///   if `bgfx::frame` is not called.
		///
		/// @warning This call should be only used on platforms that don't
		///   allow creating separate rendering thread. If it is called before
		///   to bgfx::init, render thread won't be created by bgfx::init call.
		RenderFrame::Enum renderFrame(int32_t _msecs = -1);

		void setPlatformData(const PlatformData& _data);

		struct InternalData
		{
			const struct Caps* caps; //!< Renderer capabilities.
			void* context;           //!< GL context, or D3D device.
		};

		/// Get internal data for interop.
		///
		/// @attention It's expected you understand some bgfx internals before you
		///   use this call.
		const InternalData* getInternalData();
};


#endif
