#pragma once
#ifndef __DEM_L2_AI_ACTION_H__
#define __DEM_L2_AI_ACTION_H__

#include <Core/Object.h>
#include <AI/ActorFwd.h>

// Base class of all actions executed by actors

namespace Data
{
	class CParams;
}

namespace AI
{

class CAction: public Core::CObject
{
public:

	virtual void	Init(const Data::CParams& Desc) { }
	virtual bool	Activate(CActor* pActor) { /*validate preconditions here*/ OK; }
	virtual UPTR	Update(CActor* pActor) { /*check IsComplete*/ return Success; }
	virtual void	Deactivate(CActor* pActor) { }
	
	virtual bool	IsValid(CActor* pActor) const { OK; }
	// IsInterruptible

	virtual void	GetDebugString(CString& Out) const { Out = GetClassName(); }
};

typedef Ptr<CAction> PAction;

}

#endif