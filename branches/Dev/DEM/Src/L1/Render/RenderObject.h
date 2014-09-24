#pragma once
#ifndef __DEM_L1_RENDER_OBJECT_H__
#define __DEM_L1_RENDER_OBJECT_H__

#include <Scene/NodeAttribute.h>

// Base attribute class for any renderable scene objects, like
// regular models, particle systems, terrain patches etc.

namespace Render
{

class CRenderObject: public Scene::CNodeAttribute
{
	__DeclareClassNoFactory;

public:

	virtual void UpdateInSPS() = 0;
	virtual bool ValidateResources() = 0;
};

typedef Ptr<CRenderObject> PRenderObject;

}

#endif
