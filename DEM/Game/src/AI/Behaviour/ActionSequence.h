#pragma once
#include "Action.h"
#include <Data/Array.h>

// Action sequence is a composite action that executes children in a sequence

namespace AI
{

class CActionSequence: public CAction
{
	FACTORY_CLASS_DECL;

protected:

	CArray<PAction>				Child; //???use linked list?
	CArray<PAction>::CIterator	ppCurrChild;

public:

	CActionSequence(): ppCurrChild(nullptr) {}

	void			AddChild(CAction* pAction) { n_assert(pAction); Child.Add(pAction); }
	// RemoveAllChildren / Clear (deactivate current, remove all)

	virtual void	Init(const Data::CParams& Desc);
	virtual bool	Activate(CActor* pActor);
	virtual UPTR	Update(CActor* pActor);
	virtual void	Deactivate(CActor* pActor);
	
	virtual bool	IsValid(CActor* pActor) const { return ppCurrChild && (*ppCurrChild)->IsValid(pActor); }
};

typedef Ptr<CActionSequence> PActionSequence;

}
