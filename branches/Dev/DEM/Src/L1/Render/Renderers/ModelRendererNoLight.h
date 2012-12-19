#pragma once
#ifndef __DEM_L1_RENDER_RENDERER_NL_H__
#define __DEM_L1_RENDER_RENDERER_NL_H__

#include <Render/Renderers/ModelRenderer.h>

// Model renderer, that implements rendering without lighting

namespace Render
{

class CModelRendererNoLight: public IModelRenderer
{
	DeclareRTTI;
	DeclareFactory(CModelRendererNoLight);

public:

	virtual void AddLights(const nArray<Scene::CLight*>& Lights) { /*overrides IModelRenderer, ignores lights*/ }
	virtual void Render();
};

RegisterFactory(CModelRendererNoLight);

typedef Ptr<CModelRendererNoLight> PModelRendererNoLight;

}

#endif
