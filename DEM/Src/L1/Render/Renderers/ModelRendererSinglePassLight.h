#pragma once
#ifndef __DEM_L1_RENDER_RENDERER_SPL_H__
#define __DEM_L1_RENDER_RENDERER_SPL_H__

#include <Render/Renderers/ModelRenderer.h>

// Model renderer, that implements rendering single-pass lighting with multiple lights per pass

namespace Render
{

class CModelRendererSinglePassLight: public IModelRenderer
{
	DeclareRTTI;
	DeclareFactory(CModelRendererSinglePassLight);

protected:

	//???both to light?
	bool			IsModelLitByLight(Scene::CModel& Model, Scene::CLight& Light);
	float			CalcLightPriority(Scene::CModel& Model, Scene::CLight& Light);

public:

	virtual void	Render();
};

RegisterFactory(CModelRendererSinglePassLight);

typedef Ptr<CModelRendererSinglePassLight> PModelRendererSinglePassLight;

}

#endif
