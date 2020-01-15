#pragma once
#include <Game/Property.h>
#include <Physics/StaticCollider.h>

// Trigger is an abstract level geometry that produces some events/effects on entities
// colliding with it. Trigger usually doesn't cause collision response.

// Attributes:
// int		TrgShapeType			- Collision shape type
// vector4	TrgShapeParams			- Collision shape params, see comment below for details
// float	TrgPeriod				- Period of trigger operations on entitise inside. <= 0 - once
// bool		TrgEnabled				- Is trigger enabled
// float	TrgTimeLastTriggered	- Timestamp of last trigger operation, <= 0 - never before

// TrgShapeType. TrgShapeParams:
// 1. Box:		xyz = size
// 2. Sphere:	x = radius
// 4. Capsule:	x = radius, y = length (without caps)

namespace Game
{
	typedef Ptr<class CEntity> PEntity;
}

namespace Scripting
{
	class CScriptObject;
}

namespace Prop
{
using namespace Scripting;

class CPropTrigger: public Game::CProperty
{
	FACTORY_CLASS_DECL;
	__DeclarePropertyStorage;

protected:

	Physics::PStaticCollider	CollObj;
	const CScriptObject*			pScriptObj;	// Cached pointer to CPropScriptable object
	CArray<CStrID>					CurrInsiders;
	float							Period;
	float							TimeLastTriggered;
	bool							Enabled;

	virtual bool	InternalActivate();
	virtual void	InternalDeactivate();
	void			EnableSI(class CPropScriptable& Prop);
	void			DisableSI(class CPropScriptable& Prop);

	DECLARE_EVENT_HANDLER(OnPropActivated, OnPropActivated);
	DECLARE_EVENT_HANDLER(OnPropDeactivating, OnPropDeactivating);
	DECLARE_EVENT_HANDLER(OnBeginFrame, OnBeginFrame);
	DECLARE_EVENT_HANDLER(OnLevelSaving, OnLevelSaving);
	DECLARE_EVENT_HANDLER(OnRenderDebug, OnRenderDebug);

public:

	CPropTrigger(): Enabled(false), pScriptObj(nullptr), Period(0.f), TimeLastTriggered(0.f) {}
	//virtual ~CPropTrigger();

	void SetEnabled(bool Enable);
	bool IsEnabled() const { return Enabled; }
};

}
