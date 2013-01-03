#pragma once
#ifndef __DEM_L1_RENDER_MODEL_RENDERER_H__
#define __DEM_L1_RENDER_MODEL_RENDERER_H__

#include <Render/Renderer.h>

// Model renderer is an abstract class for different model renderers. This is intended
// for different lighting implementations.

namespace Scene
{
	class CModel;
}

namespace Render
{

class IModelRenderer: public IRenderer
{
	DeclareRTTI;

protected:

	DWORD							AllowedBatchTypes;
	nArray<Scene::CModel*>			Models;
	const nArray<Scene::CLight*>*	pLights;

public:

	IModelRenderer(): pLights(NULL) {}

	virtual void AddRenderObjects(const nArray<Scene::CRenderObject*>& Objects);
	virtual void AddLights(const nArray<Scene::CLight*>& Lights);
};

typedef Ptr<IModelRenderer> PModelRenderer;

}

#endif
