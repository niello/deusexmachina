#pragma once
#include <Render/RenderFwd.h>
#define WIN32_LEAN_AND_MEAN
#define D3D_DISABLE_9EX
#if DEM_RENDER_DEBUG
#define D3D_DEBUG_INFO
#endif
#include <d3d9.h>
