#pragma once
#ifndef __DEM_L1_RENDER_RENDERER_SPL_H__
#define __DEM_L1_RENDER_RENDERER_SPL_H__

#include <Render/Renderers/ModelRenderer.h>

// Model renderer, that implements rendering single-pass lighting with multiple lights per pass

//!!!light scissors or clip planes!

namespace Render
{

class CModelRendererSinglePassLight: public IModelRenderer
{
	DeclareRTTI;
	DeclareFactory(CModelRendererSinglePassLight);

public:

	virtual void Render();
};

RegisterFactory(CModelRendererSinglePassLight);

typedef Ptr<CModelRendererSinglePassLight> PModelRendererSinglePassLight;

}

#endif
