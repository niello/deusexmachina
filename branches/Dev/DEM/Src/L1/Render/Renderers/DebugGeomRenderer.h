#pragma once
#ifndef __DEM_L1_RENDER_DBG_GEOM_RENDERER_H__
#define __DEM_L1_RENDER_DBG_GEOM_RENDERER_H__

#include <Render/Renderer.h>

// Debug geometry renderer renders shapes and primitives queued in DebugDraw

namespace Render
{

class CDebugGeomRenderer: public IRenderer
{
	DeclareRTTI;
	DeclareFactory(CDebugGeomRenderer);

public:

	virtual bool	Init(const Data::CParams& Desc) { OK; }
	virtual void	AddRenderObjects(const nArray<Scene::CRenderObject*>& Objects) {}
	virtual void	AddLights(const nArray<Scene::CLight*>& Lights) {}
	virtual void	Render();
};

RegisterFactory(CDebugGeomRenderer);

typedef Ptr<CDebugGeomRenderer> PDebugGeomRenderer;

}

#endif
