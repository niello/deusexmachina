#pragma once
#ifndef __DEM_L1_RENDER_GPU_DRIVER_H__
#define __DEM_L1_RENDER_GPU_DRIVER_H__

#include <Core/Object.h>
#include <Render/VertexComponent.h>
#include <Data/Dictionary.h>
#include <Data/StringID.h>

// GPU device driver manages VRAM resources and provides an interface for rendering on a video card.
// Implementations of this class are typically based on some graphics API, like D3D or OpenGL.

//???!!!singleton? or one per real rendering device? how multi-GPU works?
//???one render device per display adapter active?
//???one active display adapter per OS window, or per OS display, which owns on or more windows?
//!!!GPU driver can be used without display, for stream-out, render to texture or compute shaders!

namespace Render
{
typedef Ptr<class CVertexLayout> PVertexLayout;
class CVertexBuffer;
class CIndexBuffer;

class CGPUDriver: public Core::CObject
{
protected:

	//!!!see RenderSrv!

	CDict<CStrID, PVertexLayout>	VertexLayouts;

	//default RT
	//many different curr (VB, IB ...) settings
	//render stats (frame count, fps, dips, prims etc)
	//???resource manager capabilities for VRAM resources? at least can handle OnLost-OnReset right here.

public:

	CGPUDriver() {}
	virtual ~CGPUDriver() { }

	virtual CVertexLayout*	CreateVertexLayout() = 0; // Prefer GetVertexLayout() when possible
	virtual CVertexBuffer*	CreateVertexBuffer() = 0;
	virtual CIndexBuffer*	CreateIndexBuffer() = 0;
	PVertexLayout			GetVertexLayout(const CArray<CVertexComponent>& Components);
	PVertexLayout			GetVertexLayout(CStrID Signature);
};

PVertexLayout CGPUDriver::GetVertexLayout(CStrID Signature)
{
	int Idx = VertexLayouts.FindIndex(Signature);
	return Idx != INVALID_INDEX ? VertexLayouts.ValueAt(Idx) : NULL;
}
//---------------------------------------------------------------------

}

#endif
