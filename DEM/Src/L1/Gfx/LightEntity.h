#pragma once
#ifndef __DEM_L2_GFX_LIGHT_ENTITY_H__
#define __DEM_L2_GFX_LIGHT_ENTITY_H__

#include "RenderableEntity.h"
#include <gfx2/nlight.h>

// A graphics entity which emits Light into its surroundings.
// A Light entity does not need an existing Nebula2 object to work, it will
// just create it's own Light.

class nLightNode;

namespace Graphics
{

class CLightEntity: public CRenderableEntity
{
	DeclareRTTI;
	DeclareFactory(CLightEntity);

private:

	static int LightUID;

	nRef<nLightNode>	refLightNode;

public:

	nLight				Light;

	virtual void		Activate();
	virtual void		Deactivate();
	virtual void		Render();

	void				UpdateNebulaLight();

	virtual EEntityType	GetType() const { return GFXLight; }
	virtual EClipStatus	GetBoxClipStatus(const bbox3& Box);
};

RegisterFactory(CLightEntity);

}

#endif
