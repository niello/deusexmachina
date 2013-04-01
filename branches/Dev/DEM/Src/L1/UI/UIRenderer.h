#pragma once
#ifndef __DEM_L1_UI_RENDERER_H__
#define __DEM_L1_UI_RENDERER_H__

#include <Render/Renderer.h>
#include <Render/Materials/Shader.h>
#include <Scene/Model.h>

// This UI renderer renders CEGUI

namespace Render
{

class CUIRenderer: public IRenderer
{
	DeclareRTTI;
	DeclareFactory(CUIRenderer);

protected:

public:

	//CUIRenderer(): pLights(NULL), FeatFlags(0), DistanceSorting(Sort_None), EnableLighting(false) {}

	virtual bool	Init(const Data::CParams& Desc) { OK; }
	virtual void	AddRenderObjects(const nArray<Scene::CRenderObject*>& Objects) {}
	virtual void	AddLights(const nArray<Scene::CLight*>& Lights) {}
	virtual void	Render();
};

RegisterFactory(CUIRenderer);

typedef Ptr<CUIRenderer> PUIRenderer;

}

#endif
