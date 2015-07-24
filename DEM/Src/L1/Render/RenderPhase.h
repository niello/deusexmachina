#pragma once
#ifndef __DEM_L1_RENDER_PHASE_H__
#define __DEM_L1_RENDER_PHASE_H__

#include <Core/Object.h>
#include <Data/FixedArray.h>
#include <Data/Flags.h>

// Rendering phase implements a process of rendering into RT, MRT or DSV, in various forms,
// including color rendering, Z-prepass, occlusion culling, shadow map, fullscreen quad for
// postprocessing effects and others.

namespace Data
{
	class CParams;
}

namespace Render
{
class CCamera;
class CSPS;
class CRenderPath;
typedef Ptr<class CRenderTarget> PRenderTarget;
typedef Ptr<class CDepthStencilBuffer> PDepthStencilBuffer;

class CRenderPhase: public Core::CObject
{
public:

	CFixedArray<PRenderTarget>	RenderTargets;
	PDepthStencilBuffer			DepthStencil;

	Data::CFlags				ClearFlags;
	CFixedArray<DWORD>			RTClearColors;		// 32-bit ARGB each
	float						DepthClearValue;	// 0.f .. 1.f
	uchar						StencilClearValue;

	CRenderPhase(): ClearFlags(0), DepthClearValue(1.f), StencilClearValue(0) {}

	virtual ~CRenderPhase() {}

	virtual bool Init(const Data::CParams& Desc, const CRenderPath& Owner);
	virtual bool Render(const CCamera& MainCamera, CSPS& SPS, const CRenderPath& Owner) = 0;
};

typedef Ptr<CRenderPhase> PRenderPhase;

}

#endif
