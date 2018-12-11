#ifndef MYGFX_P_H_HEADER_GUARD
#define MYGFX_P_H_HEADER_GUARD

#include <bx/platform.h>

#ifndef MYGFX_CONFIG_DEBUG
#	define MYGFX_CONFIG_DEBUG 0
#endif // MYGFX_CONFIG_DEBUG

#if MYGFX_CONFIG_DEBUG || BX_COMPILER_CLANG_ANALYZER
#	define BX_TRACE _BX_TRACE
#	define BX_WARN  _BX_WARN
#	define BX_CHECK _BX_CHECK
#	define BX_CONFIG_ALLOCATOR_DEBUG 1
#endif // MYGFX_CONFIG_DEBUG

#include "mygfx/mygfx.h"
#include "config.h"

#include <inttypes.h>










#endif