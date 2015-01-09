#pragma once
#ifndef __DEM_L1_RENDER_OBJECT_H__
#define __DEM_L1_RENDER_OBJECT_H__

#include <Scene/NodeAttribute.h>

// Base attribute class for any renderable scene objects, like
// regular models, particle systems, terrain patches etc.

namespace Render
{
class CSPS;

class CRenderObject: public Scene::CNodeAttribute
{
	__DeclareClassNoFactory;

protected:

	enum // extends Scene::CNodeAttribute enum
	{
		//AddedAsAlwaysVisible	= 0x04,	// To avoid searching in SPS AlwaysVisible array at each UpdateInSPS() call
		DoOcclusionCulling		= 0x08,
		CastShadow				= 0x10,
		ReceiveShadow			= 0x20 //???needed for some particle systems?
	};

public:

	virtual void UpdateInSPS(CSPS& SPS) = 0;
	virtual bool ValidateResources() = 0;
};

typedef Ptr<CRenderObject> PRenderObject;

}

#endif
