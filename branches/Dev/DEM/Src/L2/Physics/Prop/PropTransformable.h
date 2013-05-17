#pragma once
#ifndef __DEM_L2_PROP_TRANSFORMABLE_H__
#define __DEM_L2_PROP_TRANSFORMABLE_H__

#include <Game/Property.h>
#include <db/AttrID.h>

// Entities with this property can be transformed.
// DEM approach to entity transformation:
// TF sources (like input, code, CPropPathAnim) =>
// SetTransform event =>
// TF handlers (CPropTransformable and derivatives like physics props) =>
// set CStrID("Transform") based on internal logic + SetTransform, fire OnTransformChanged =>
// TF receivers (gfx, cameras, lights, audio src etc) => update TF of components
// This allows to attach path animation to physics entities

namespace Attr
{
	DeclareMatrix44(Transform);
};

namespace Properties
{

class CPropTransformable: public Game::CProperty
{
	__DeclareClass(CPropTransformable);
	__DeclarePropertyStorage;

protected:

	DECLARE_EVENT_HANDLER(SetTransform, OnSetTransform);
	DECLARE_EVENT_HANDLER_VIRTUAL(OnRenderDebug, OnRenderDebug);

	virtual void SetTransform(const matrix44& NewTF);

public:

	virtual void GetAttributes(nArray<DB::CAttrID>& Attrs);
	virtual void Activate();
	virtual void Deactivate();
};

} // namespace Properties

#endif
