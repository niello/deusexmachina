#pragma once
#ifndef __DEM_L2_AI_ACTION_SEQUENCE_H__
#define __DEM_L2_AI_ACTION_SEQUENCE_H__

#include "Action.h"
#include <util/narray.h>

// Action sequence is a composite action that executes children in a sequence.

namespace AI
{

class CActionSequence: public CAction
{
protected:

	nArray<PAction>				Child; //???use linked list?
	nArray<PAction>::CIterator	ppCurrChild;

public:

	CActionSequence(): ppCurrChild(NULL) {}

	void				AddChild(CAction* pAction) { n_assert(pAction); Child.Append(pAction); }
	// RemoveAllChildren / Clear (deactivate current, remove all)

	virtual bool		Activate(CActor* pActor);
	virtual EExecStatus	Update(CActor* pActor);
	virtual void		Deactivate(CActor* pActor);
	
	virtual bool		IsValid(CActor* pActor) const { return ppCurrChild && (*ppCurrChild)->IsValid(pActor); }
};

typedef Ptr<CActionSequence> PActionSequence;

}

#endif