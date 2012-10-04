#pragma once
#ifndef __DEM_L2_PROP_GRAPHICS_H__
#define __DEM_L2_PROP_GRAPHICS_H__

#include "PropAbstractGraphics.h"

// This is a simple graphics property which adds visibility to a game entity.

namespace Graphics
{
	typedef Ptr<class CShapeEntity> PShapeEntity;
};

namespace Properties
{

class CPropSimpleGraphics: public CPropAbstractGraphics
{
	DeclareRTTI;
	DeclareFactory(CPropSimpleGraphics);
	DeclarePropertyStorage;
	DeclarePropertyPools(Game::LivePool);

protected:

	Graphics::PShapeEntity GfxEntity;

	DECLARE_EVENT_HANDLER(UpdateTransform, OnUpdateTransform);
	DECLARE_EVENT_HANDLER(GfxSetVisible, OnGfxSetVisible);
	DECLARE_EVENT_HANDLER(OnEntityRenamed, OnEntityRenamed);

public:

	CPropSimpleGraphics();
	virtual ~CPropSimpleGraphics();

	virtual void GetAttributes(nArray<DB::CAttrID>& Attrs);
	virtual void Activate();
	virtual void Deactivate();

	Graphics::CShapeEntity*	GetGraphicsEntity() const { return GfxEntity; }
};

RegisterFactory(CPropSimpleGraphics);

}

#endif
