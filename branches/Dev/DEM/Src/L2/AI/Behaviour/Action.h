#pragma once
#ifndef __DEM_L2_AI_ACTION_H__
#define __DEM_L2_AI_ACTION_H__

#include <StdDEM.h>
#include <Core/RefCounted.h>
#include <AI/ActorFwd.h>

// Action stores action template and local context. Bhv system executes these actions.

namespace AI
{
class CActionTpl;

class CAction: public Core::CRefCounted
{
private:

	const CActionTpl* pTpl; //???is really needed here? now used only in planning! if need, smart ptr?

public:

	CAction(): pTpl(NULL) {}
	CAction(const CActionTpl* pTemplate): pTpl(pTemplate) { }

	virtual bool		Activate(CActor* pActor) { /*validate preconditions here*/ OK; }
	virtual EExecStatus	Update(CActor* pActor) { /*check IsComplete*/ return Success; }
	virtual void		Deactivate(CActor* pActor) { }
	
	virtual bool		IsValid(CActor* pActor) const { OK; }
	// IsInterruptible

	virtual void		GetDebugString(CString& Out) const { Out = GetClassName(); }
};

typedef Ptr<CAction> PAction;

}

#endif