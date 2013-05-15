#pragma once
#ifndef __DEM_L2_PROP_TRIGGER_H__
#define __DEM_L2_PROP_TRIGGER_H__

#include "PropTransformable.h"
#include <DB/AttrID.h>

// Trigger is an abstract level geometry that produces some events/effects on entities
// colliding with it

namespace Physics
{
	typedef Ptr<class CShape> PShape;
}

namespace Game
{
	typedef Ptr<class CEntity> PEntity;
};

namespace Scripting
{
	class CScriptObject;
}

namespace Attr
{
	DeclareInt(TrgShapeType);			// Collision shape type
	DeclareFloat4(TrgShapeParams);		// Collision shape params, see comment below for details
	DeclareFloat(TrgPeriod);			// Period of trigger operations on entitise inside. <= 0 - once
	DeclareBool(TrgEnabled);			// Is trigger enabled //???editor complications? to TrgShapeType's most significant bit
	DeclareFloat(TrgTimeLastTriggered);	// Timestamp of last trigger operation, <= 0 - never before
};

// Collision shape params (float4):
// Box:		xyz = size
// Sphere:	x = radius
// Plane:	nothing
// Capsule: x = radius, y = length (without caps)
// Remember that position & orientation are in Transform parameter (box mb can also use scaling...)
// Float4 isn't enough for mesh shape (string would suffice, but now we needn't mesh-shaped triggers)

namespace Properties
{
using namespace Scripting;

class CPropTrigger: public CPropTransformable
{
	__DeclareClass(CPropTrigger);

protected:

	bool					Enabled;
	Physics::PShape			pCollShape; //???allow many shapes?
	nArray<Game::PEntity>	EntitiesInsideNow;
	nArray<Game::PEntity>	EntitiesInsideLastFrame;
	bool					SwapArrays;
	const CScriptObject*	pScriptObj;
	float					Period;
	float					TimeLastTriggered; //???use nTime?

	DECLARE_EVENT_HANDLER(OnPropsActivated, OnPropsActivated);
	DECLARE_EVENT_HANDLER(ExposeSI, ExposeSI);
	DECLARE_EVENT_HANDLER(OnBeginFrame, OnBeginFrame);
	DECLARE_EVENT_HANDLER(OnSave, OnSave);
	DECLARE_EVENT_HANDLER(OnRenderDebug, OnRenderDebug);

public:

	CPropTrigger(): Enabled(false), SwapArrays(false), pScriptObj(NULL), Period(0.f), TimeLastTriggered(0.f) {}
	virtual ~CPropTrigger();

	virtual void	Activate();
	virtual void	Deactivate();
	virtual void	GetAttributes(nArray<DB::CAttrID>& Attrs);

	void			SetEnabled(bool Enable);
	bool			IsEnabled() const { return Enabled; }
};

__RegisterClassInFactory(CPropTrigger);

}

#endif
