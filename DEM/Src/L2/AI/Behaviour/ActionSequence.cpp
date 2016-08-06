#include "ActionSequence.h"

#include <AI/AIServer.h>
#include <Data/DataArray.h>
#include <Core/Factory.h>

namespace AI
{
__ImplementClass(AI::CActionSequence, 'ASEQ', AI::CAction)

void CActionSequence::Init(const Data::CParams& Desc)
{
	const Data::CDataArray& ChildDescs = *Desc.Get<Data::PDataArray>(CStrID("Child"));
	for (UPTR i = 0; i < ChildDescs.GetCount(); ++i)
		AddChild(AI::CAIServer::CreatePlanFromDesc(ChildDescs.Get<Data::PParams>(i)));
}
//---------------------------------------------------------------------

bool CActionSequence::Activate(CActor* pActor)
{
	n_assert(!ppCurrChild);
	if (Child.GetCount() < 1) FAIL;
	CArray<PAction>::CIterator ItFirst = Child.Begin();
	if (!(*ItFirst)->Activate(pActor)) FAIL;
	ppCurrChild = ItFirst;
	OK;
}
//---------------------------------------------------------------------

UPTR CActionSequence::Update(CActor* pActor)
{
	n_assert(ppCurrChild);

	while (true)
	{
		UPTR Result = (*ppCurrChild)->Update(pActor);

		if (Result == Running) return Running;

		(*ppCurrChild)->Deactivate(pActor);
		
		if (Result == Failure) return Failure;
		
		if (++ppCurrChild == Child.End())
		{
			ppCurrChild = NULL;
			return Success;
		}
		
		if (!(*ppCurrChild)->Activate(pActor)) return Failure;
	}
}
//---------------------------------------------------------------------

void CActionSequence::Deactivate(CActor* pActor)
{
	if (ppCurrChild)
	{
		(*ppCurrChild)->Deactivate(pActor);
		ppCurrChild = NULL;
	}
}
//---------------------------------------------------------------------

}