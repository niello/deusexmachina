#pragma once
#ifndef __DEM_L1_RENDER_RENDERER_H__
#define __DEM_L1_RENDER_RENDERER_H__

#include <Core/RefCounted.h>
//#include <Data/StringID.h>
//#include <Render/Render.h>

// Renderer is responsible for rendering certain type of graphics elements, like meshes,
// particles, terrain patches, debug shapes, text, UI etc. Renderer can be fed directly
// and by frame shader (with visible scene node attributes). Renderer should use RenderSrv
// methods to access hardware graphics device functionality.

namespace Scene
{
	class CRenderObject;
	class CLight;
}

namespace Render
{

class IRenderer: public Core::CRefCounted
{
	DeclareRTTI;

public:

	virtual void AddRenderObjects(const nArray<Scene::CRenderObject*>& Objects) = 0;
	virtual void AddLights(const nArray<Scene::CLight*>& Lights) = 0;
	virtual void Render() = 0;
};

typedef Ptr<IRenderer> PRenderer;

}

#endif
