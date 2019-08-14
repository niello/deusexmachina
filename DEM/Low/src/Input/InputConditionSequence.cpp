#include "InputConditionSequence.h"
#include <Input/InputEvents.h>
#include <Input/InputDevice.h>
#include <Input/InputConditionUp.h>

namespace Input
{
__ImplementClassNoFactory(Input::CInputConditionSequence, Input::CInputConditionEvent);

void CInputConditionSequence::AddChild(PInputConditionEvent&& NewChild)
{
	if (!NewChild || NewChild.get() == this) return;
	Children.push_back(std::move(NewChild));
}
//---------------------------------------------------------------------

void CInputConditionSequence::Clear()
{
	Children.clear();
	CurrChild = 0;
}
//---------------------------------------------------------------------

void CInputConditionSequence::Reset()
{
	for (auto& Child : Children)
		Child->Reset();
	CurrChild = 0;
}
//---------------------------------------------------------------------

bool CInputConditionSequence::OnAxisMove(const IInputDevice* pDevice, const Event::AxisMove& Event)
{
	if (CurrChild >= Children.size()) FAIL;

	if (Children[CurrChild]->OnAxisMove(pDevice, Event))
	{
		++CurrChild;
		if (CurrChild == Children.size())
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
	if (CurrChild >= Children.size()) FAIL;

	CInputConditionEvent* pCurrEvent = Children[CurrChild].get();
	if (pCurrEvent->OnButtonDown(pDevice, Event))
	{
		++CurrChild;
		if (CurrChild == Children.size())
		{
			CurrChild = 0;
			OK;
		}
	}
	else
	{
		// If we wait Up and this is Down of the same button, we don't break the sequence
		// FIXME: Down-and-Up event needed, CInputConditionClick
		if (auto pUpEvent = pCurrEvent->As<CInputConditionUp>())
		{
			if (pUpEvent->GetDeviceType() == pDevice->GetType() && pUpEvent->GetButton() == Event.Code) FAIL;
		}

		CurrChild = 0; // Reset on any other unexpected ButtonDown
	}

	FAIL;
}
//---------------------------------------------------------------------

bool CInputConditionSequence::OnButtonUp(const IInputDevice* pDevice, const Event::ButtonUp& Event)
{
	if (CurrChild >= Children.size()) FAIL;

	if (Children[CurrChild]->OnButtonUp(pDevice, Event))
	{
		++CurrChild;
		if (CurrChild == Children.size())
		{
			CurrChild = 0;
			OK;
		}
	}
	else CurrChild = 0; // Reset on any unexpected ButtonUp

	FAIL;
}
//---------------------------------------------------------------------

bool CInputConditionSequence::OnTextInput(const IInputDevice* pDevice, const Event::TextInput& Event)
{
	if (CurrChild >= Children.size()) FAIL;

	if (Children[CurrChild]->OnTextInput(pDevice, Event))
	{
		++CurrChild;
		if (CurrChild == Children.size())
		{
			CurrChild = 0;
			OK;
		}
	}
	else CurrChild = 0; // Reset on any unexpected TextInput

	FAIL;
}
//---------------------------------------------------------------------

bool CInputConditionSequence::OnTimeElapsed(float ElapsedTime)
{
	if (CurrChild >= Children.size()) FAIL;

	if (Children[CurrChild]->OnTimeElapsed(ElapsedTime))
	{
		++CurrChild;
		if (CurrChild == Children.size())
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