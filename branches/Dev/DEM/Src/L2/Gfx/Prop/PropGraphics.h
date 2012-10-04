#pragma once
#ifndef __DEM_L2_PROP_GRAPHICS_H__
#define __DEM_L2_PROP_GRAPHICS_H__

#include "PropAbstractGraphics.h"
#include <mathlib/bbox.h>

// This is the standard graphics property which adds visibility to a game entity.
// Based on mangalore GraphicsProperty (C) 2005 Radon Labs GmbH

// NOTE: There are cases where the graphics property may depend on a
// physics property (for complex physics entities which require several
// graphics entities to render themselves). Thus it is recommended that
// physics properties are attached before graphics properties.

namespace Graphics
{
    typedef Ptr<class CShapeEntity> PShapeEntity;
};

namespace Properties
{
using namespace Events;

typedef nArray<Graphics::PShapeEntity> CGfxShapeArray;

class CPropGraphics: public CPropAbstractGraphics
{
	DeclareRTTI;
	DeclareFactory(CPropGraphics);
	DeclarePropertyStorage;
	DeclarePropertyPools(Game::LivePool);

protected:

	CGfxShapeArray GraphicsEntities;

	DECLARE_EVENT_HANDLER(UpdateTransform, OnUpdateTransform);
	DECLARE_EVENT_HANDLER(GfxSetVisible, OnGfxSetVisible);
	DECLARE_EVENT_HANDLER(OnEntityRenamed, OnEntityRenamed);

	virtual void	SetupGraphicsEntities();
	virtual void	UpdateTransform();

public:

	virtual ~CPropGraphics();

	virtual void			GetAttributes(nArray<DB::CAttrID>& Attrs);
	virtual void			Activate();
	virtual void			Deactivate();

	const CGfxShapeArray&	GetGfxEntities() const { return GraphicsEntities; }
	void					GetAABB(bbox3& AABB) const;
};

RegisterFactory(CPropGraphics);

} // namespace Properties

#endif
