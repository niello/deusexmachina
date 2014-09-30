#pragma once
#ifndef __DEM_L1_UI_RENDERER_H__
#define __DEM_L1_UI_RENDERER_H__

#include <Render/Renderer.h>

// This UI renderer renders CEGUI

namespace Render
{

class CUIRenderer: public IRenderer
{
	__DeclareClass(CUIRenderer);

protected:

public:

	//CUIRenderer(): pLights(NULL), FeatFlags(0), DistanceSorting(Sort_None), EnableLighting(false) {}

	virtual bool	Init(const Data::CParams& Desc) { OK; }
	virtual void	AddRenderObjects(const CArray<CRenderObject*>& Objects) {}
	virtual void	AddLights(const CArray<CLight*>& Lights) {}
	virtual void	Render();
};

typedef Ptr<CUIRenderer> PUIRenderer;

}

#endif
