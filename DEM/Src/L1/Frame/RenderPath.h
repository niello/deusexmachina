#pragma once
#ifndef __DEM_L1_FRAME_RENDER_PATH_H__
#define __DEM_L1_FRAME_RENDER_PATH_H__

#include <Core/Object.h>
#include <Data/FixedArray.h>
#include <Data/Array.h>

// Render path incapsulates a full algorithm to render a frame, allowing to
// define it in a data-driven manner and to avoid hardcoding frame rendering.
// It describes, how to use what shaders on what objects. The final output
// is a complete frame, rendered in an output render target.
// Render path consists of phases, each of which fills RT, MRT and/or DS,
// or does some intermediate processing, like an occlusion culling.
// Render path could be designed for some feature level (DX9, DX11), for some
// rendering concept (forward, deferred), for different features used (HDR) etc.

namespace Data
{
	class CParams;
}

namespace Frame
{
typedef Ptr<class CRenderPhase> PRenderPhase;

class CRenderPath: public Core::CObject
{
public:

	//!!!global shader variables!

	CFixedArray<PRenderPhase>	Phases;

	//bool Init(CGPUDriver& Driver, const Data::CParams& Desc);
	//bool Render(const CCamera& MainCamera, CSPS& SPS);
};

typedef Ptr<CRenderPath> PRenderPath;

}

#endif
