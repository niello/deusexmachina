#pragma once
#ifndef __DEM_L2_AI_ACTION_GOTO_SMART_OBJ_H__
#define __DEM_L2_AI_ACTION_GOTO_SMART_OBJ_H__

#include <AI/Movement/Actions/ActionGoto.h>
#include <Data/StringID.h>

// Goto action that sets destination from smart object, calculating destination.

namespace AI
{

class CActionGotoSmartObj: public CActionGoto
{
	__DeclareClass(CActionGotoSmartObj);

private:

	CStrID TargetID;
	CStrID ActionID;

	//???can be dynamic?

public:

	void			Init(CStrID Target, CStrID Action) { TargetID = Target; ActionID = Action; }
	virtual bool	Activate(CActor* pActor);

	virtual void	GetDebugString(nString& Out) const { Out.Format("%s(%s, %s)", GetClassName().Get(), TargetID.CStr(), ActionID.CStr()); }
};

RegisterFactory(CActionGotoSmartObj);

typedef Ptr<CActionGotoSmartObj> PActionGotoSmartObj;

}

#endif