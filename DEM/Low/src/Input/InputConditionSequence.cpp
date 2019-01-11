#include "InputConditionSequence.h"

#include <Input/InputEvents.h>
#include <Input/InputDevice.h>
#include <Input/InputConditionUp.h>
#include <Data/DataArray.h>
#include <Core/Factory.h>

namespace Input
{
__ImplementClass(Input::CInputConditionSequence, 'ICSQ', Input::CInputConditionEvent);


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
		if (!pEvent || !pEvent->Initialize(*SubDesc.Get())) FAIL;
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

void CInputConditionSequence::Reset()
{
	for (UPTR i = 0; i < Children.GetCount(); ++i)
		if (Children[i]) Children[i]->Reset();
	CurrChild = 0;
}
//---------------------------------------------------------------------

bool CInputConditionSequence::OnAxisMove(const IInputDevice* pDevice, const Event::AxisMove& Event)
{
	if (CurrChild >= Children.GetCount()) FAIL;

	if (Children[CurrChild]->OnAxisMove(pDevice, Event))
	{
		++CurrChild;
		if (CurrChild == Children.GetCount())
		{
			CurrChild = 0;
			OK;
		}
	}

	// We never reset on AxisMove for now to prevent sequence breaking on unintended mouse moves etc

	FAIL;
}
//---------------------------------------------------------------------

bool CInputConditionSequence::OnButtonDown(const IInputDevice* pDevice, const Event::ButtonDown& Event)
{
	if (CurrChild >= Children.GetCount()) FAIL;

	CInputConditionEvent* pCurrEvent = Children[CurrChild];
	if (pCurrEvent->OnButtonDown(pDevice, Event))
	{
		++CurrChild;
		if (CurrChild == Children.GetCount())
		{
			CurrChild = 0;
			OK;
		}
	}
	else
	{
		// If we wait Up and this is Down of the same button, we don't break the sequence
		if (pCurrEvent->IsA<CInputConditionUp>()&&
			((CInputConditionUp*)pCurrEvent)->GetDeviceType() == pDevice->GetType() &&
			((CInputConditionUp*)pCurrEvent)->GetButton() == Event.Code)
		{
			FAIL;
		}

		CurrChild = 0; // Reset on any other unexpected ButtonDown
	}

	FAIL;
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
	else CurrChild = 0; // Reset on any unexpected ButtonUp

	FAIL;
}
//---------------------------------------------------------------------

bool CInputConditionSequence::OnTimeElapsed(float ElapsedTime)
{
	if (CurrChild >= Children.GetCount()) FAIL;

	if (Children[CurrChild]->OnTimeElapsed(ElapsedTime))
	{
		++CurrChild;
		if (CurrChild == Children.GetCount())
		{
			CurrChild = 0;
			OK;
		}
	}

	// Sequence timeout can be added, then reset will be performed here

	FAIL;
}
//---------------------------------------------------------------------

}