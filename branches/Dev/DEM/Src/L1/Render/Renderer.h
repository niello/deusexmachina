#pragma once
#ifndef __DEM_L1_RENDER_RENDERER_H__
#define __DEM_L1_RENDER_RENDERER_H__

#include <Core/Object.h>
#include <Data/Params.h>

// Renderer is responsible for rendering certain type of graphics elements, like meshes,
// particles, terrain patches, debug shapes, text, UI etc. Renderer can be fed directly
// and by frame shader (with visible scene node attributes). Renderer should use RenderSrv
// methods to access hardware graphics device functionality.
// Use renderers to implement different rendering strategies on CPU, and use shader for GPU variations.

namespace Render
{
class CRenderObject;
class CLight;

class IRenderer: public Core::CObject
{
	__DeclareClassNoFactory;

public:

	virtual ~IRenderer() {}

	virtual bool Init(const Data::CParams& Desc) = 0;
	virtual void AddRenderObjects(const CArray<CRenderObject*>& Objects) = 0;
	virtual void AddLights(const CArray<CLight*>& Lights) = 0;
	virtual void Render() = 0;
};

typedef Ptr<IRenderer> PRenderer;

}

#endif
