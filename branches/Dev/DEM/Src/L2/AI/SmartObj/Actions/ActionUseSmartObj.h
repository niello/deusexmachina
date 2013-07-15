#pragma once
#ifndef __DEM_L2_AI_ACTION_USE_SMART_OBJ_H__
#define __DEM_L2_AI_ACTION_USE_SMART_OBJ_H__

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
class CSmartObjAction;

class CActionUseSmartObj: public CAction
{
	__DeclareClass(CActionUseSmartObj);

private:

	CStrID							TargetID;
	CStrID							ActionID;

	bool							WasDone;

	Prop::CPropSmartObject*	pSO;
	CSmartObjAction*				Action;

	PAction							SubActFace;

	void				StartSOAction(CActor* pActor);

public:

	void				Init(CStrID Target, CStrID Action) { TargetID = Target; ActionID = Action; }
	virtual bool		Activate(CActor* pActor);
	virtual EExecStatus	Update(CActor* pActor);
	virtual void		Deactivate(CActor* pActor);
	virtual bool		IsValid(CActor* pActor) const;

	virtual void		GetDebugString(CString& Out) const { Out.Format("%s(%s, %s)", GetClassName().CStr(), TargetID.CStr(), ActionID.CStr()); }
};

typedef Ptr<CActionUseSmartObj> PActionUseSmartObj;

}

#endif
