#pragma once
#ifndef __DEM_L1_RENDER_RENDERER_H__
#define __DEM_L1_RENDER_RENDERER_H__

#include <Core/Object.h>
#include <Data/Params.h>

// Renderer is responsible for rendering certain type of 3D graphics elements, like models,
// particles, terrain patches, debug shapes etc. Renderer can be fed directly or by render path
// (with visible scene node attributes). Renderer should use GPUDriver methods to access hardware
// graphics device functionality. Use renderers to implement different rendering techniques on the
// CPU side, and use shaders for GPU variations.
// Non-scene elements like debug text and UI aren't 3D objects and therefore are rendered through
// specialized render phases.

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
