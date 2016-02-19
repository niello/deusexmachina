#pragma once
#ifndef __DEM_L1_FRAME_TERRAIN_RENDERER_H__
#define __DEM_L1_FRAME_TERRAIN_RENDERER_H__

#include <Render/Renderer.h>

// Default renderer for CTerrain render objects

namespace Render
{

class CTerrainRenderer: public IRenderer
{
	__DeclareClass(CTerrainRenderer);

protected:

	UPTR	InputSet_CDLOD;

public:

	CTerrainRenderer();

	virtual CArray<CRenderNode>::CIterator Render(CArray<CRenderNode>& RenderQueue, CArray<CRenderNode>::CIterator ItCurr);
};

}

#endif
