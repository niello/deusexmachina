#pragma once
#include <AI/Behaviour/Action.h>
#include <Data/StringID.h>

// Performs action on smart object.
// Custom Use actions are highly extendable and flexible, allowing designers to implement
// any specific logic without touching C++. All setup is done with params and scripts.

namespace Prop
{
	class CPropSmartObject;
}

namespace AI
{

class CActionUseSmartObj: public CAction
{
	FACTORY_CLASS_DECL;

private:

	CStrID	TargetID;
	CStrID	ActionID;
	float	Duration;
	float	Progress;
	bool	WasDone;

	UPTR			SetDone(CActor* pActor, Prop::CPropSmartObject* pSO, const class CSmartAction& ActTpl);

public:

	void			Init(CStrID Target, CStrID Action) { TargetID = Target; ActionID = Action; }
	virtual bool	Activate(CActor* pActor);
	virtual UPTR	Update(CActor* pActor);
	virtual void	Deactivate(CActor* pActor);
	virtual bool	IsValid(CActor* pActor) const;

	//virtual void	GetDebugString(std::string& Out) const { Out.Format("%s(%s, %s)", GetClassName().CStr(), TargetID.CStr(), ActionID.CStr()); }
};

typedef Ptr<CActionUseSmartObj> PActionUseSmartObj;

}
