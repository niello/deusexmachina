#pragma once
#ifndef __DEM_L1_RENDER_RENDERER_MPL_H__
#define __DEM_L1_RENDER_RENDERER_MPL_H__

#include <Render/Renderers/ModelRenderer.h>

// Model renderer, that implements multipass lighting with one light per pass

namespace Render
{

class CModelRendererMultiPassLight: public IModelRenderer
{
	DeclareRTTI;
	DeclareFactory(CModelRendererMultiPassLight);

public:

	virtual void Render();
};

RegisterFactory(CModelRendererMultiPassLight);

typedef Ptr<CModelRendererMultiPassLight> PModelRendererMultiPassLight;

}

#endif
