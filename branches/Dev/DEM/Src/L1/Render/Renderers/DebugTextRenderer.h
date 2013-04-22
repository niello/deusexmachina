#pragma once
#ifndef __DEM_L1_RENDER_DBG_TEXT_RENDERER_H__
#define __DEM_L1_RENDER_DBG_TEXT_RENDERER_H__

#include <Render/Renderer.h>

// Debug text renderer renders text elements queued in DebugDraw

namespace Render
{

class CDebugTextRenderer: public IRenderer
{
	DeclareRTTI;
	DeclareFactory(CDebugTextRenderer);

public:

	virtual bool	Init(const Data::CParams& Desc) { OK; }
	virtual void	AddRenderObjects(const nArray<Scene::CRenderObject*>& Objects) {}
	virtual void	AddLights(const nArray<Scene::CLight*>& Lights) {}
	virtual void	Render();
};

RegisterFactory(CDebugTextRenderer);

typedef Ptr<CDebugTextRenderer> PDebugTextRenderer;

}

#endif
