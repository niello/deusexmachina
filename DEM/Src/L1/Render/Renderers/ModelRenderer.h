#pragma once
#ifndef __DEM_L1_RENDER_MODEL_RENDERER_H__
#define __DEM_L1_RENDER_MODEL_RENDERER_H__

#include <Render/Renderer.h>

// Model renderer is an abstract class for different model renderers. This is intended
// for different lighting implementations.

namespace Render
{

class IModelRenderer: public IRenderer
{
	DeclareRTTI;

public:

	virtual void AddRenderObjects(const nArray<Scene::CRenderObject*>& Objects);
	virtual void AddLights(const nArray<Scene::CLight*>& Lights);
};

typedef Ptr<IModelRenderer> PModelRenderer;

}

#endif
