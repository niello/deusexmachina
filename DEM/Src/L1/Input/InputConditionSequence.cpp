#include "InputConditionSequence.h"

#include <Input/InputEvents.h>
#include <Input/InputDevice.h>
#include <Data/DataArray.h>

namespace Input
{

bool CInputConditionSequence::Initialize(const Data::CParams& Desc)
{
	Clear();

	Data::PDataArray EventDescArray;
	if (!Desc.Get<Data::PDataArray>(EventDescArray, CStrID("Events"))) OK;

	Children.SetSize(EventDescArray->GetCount());
	for (UPTR i = 0; i < Children.GetCount(); ++i)
	{
		Data::PParams SubDesc = EventDescArray->Get<Data::PParams>(i);
		CInputConditionEvent* pEvent = CInputConditionEvent::CreateByType(SubDesc->Get<CString>(CStrID("Type")));
		if (!pEvent || !pEvent->Initialize(*SubDesc.GetUnsafe())) FAIL;
		Children[i] = pEvent;
	}

	OK;
}
//---------------------------------------------------------------------

void CInputConditionSequence::Clear()
{
	for (UPTR i = 0; i < Children.GetCount(); ++i)
		if (Children[i]) n_delete(Children[i]);
	Children.SetSize(0);
	CurrChild = 0;
}
//---------------------------------------------------------------------

bool CInputConditionSequence::OnButtonUp(const IInputDevice* pDevice, const Event::ButtonUp& Event)
{
	if (CurrChild >= Children.GetCount()) FAIL;

	if (Children[CurrChild]->OnButtonUp(pDevice, Event))
	{
		++CurrChild;
		if (CurrChild == Children.GetCount())
		{
			CurrChild = 0;
			OK;
		}
	}
	else CurrChild = 0; //???!!!when reset? up may reset down of the same key, it is not always desirable!

	//!!!need a set of resetting flags, like ResetOnOtherDown = true

	FAIL;
}
//---------------------------------------------------------------------

}