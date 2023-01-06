#include "InputConditionSequence.h"
#include <Input/InputEvents.h>
#include <Input/InputDevice.h>
#include <Input/InputConditionUp.h>

namespace Input
{

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

bool CInputConditionSequence::UpdateParams(std::function<std::string(const char*)> ParamGetter, std::set<std::string>* pOutParams)
{
	bool Result = true;
	for (auto& Child : Children)
		Result &= Child->UpdateParams(ParamGetter, pOutParams);
	return Result;
}
//---------------------------------------------------------------------

void CInputConditionSequence::Reset()
{
	for (auto& Child : Children)
		Child->Reset();
	CurrChild = 0;
}
//---------------------------------------------------------------------

UPTR CInputConditionSequence::OnAxisMove(const IInputDevice* pDevice, const Event::AxisMove& Event)
{
	if (CurrChild >= Children.size()) return 0;

	if (Children[CurrChild]->OnAxisMove(pDevice, Event))
	{
		if (++CurrChild == Children.size())
		{
			CurrChild = 0;
			return 1;
		}
	}

	// We never reset on AxisMove for now to prevent sequence breaking on unintended mouse moves etc

	return 0;
}
//---------------------------------------------------------------------

UPTR CInputConditionSequence::OnButtonDown(const IInputDevice* pDevice, const Event::ButtonDown& Event)
{
	if (CurrChild >= Children.size()) return 0;

	CInputConditionEvent* pCurrEvent = Children[CurrChild].get();
	if (pCurrEvent->OnButtonDown(pDevice, Event))
	{
		if (++CurrChild == Children.size())
		{
			CurrChild = 0;
			return 1;
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

	return 0;
}
//---------------------------------------------------------------------

UPTR CInputConditionSequence::OnButtonUp(const IInputDevice* pDevice, const Event::ButtonUp& Event)
{
	if (CurrChild >= Children.size()) return 0;

	if (Children[CurrChild]->OnButtonUp(pDevice, Event))
	{
		if (++CurrChild == Children.size())
		{
			CurrChild = 0;
			return 1;
		}
	}
	else CurrChild = 0; // Reset on any unexpected ButtonUp

	return 0;
}
//---------------------------------------------------------------------

UPTR CInputConditionSequence::OnTextInput(const IInputDevice* pDevice, const Event::TextInput& Event)
{
	if (CurrChild >= Children.size()) return 0;

	if (Children[CurrChild]->OnTextInput(pDevice, Event))
	{
		if (++CurrChild == Children.size())
		{
			CurrChild = 0;
			return 1;
		}
	}
	else CurrChild = 0; // Reset on any unexpected TextInput

	return 0;
}
//---------------------------------------------------------------------

UPTR CInputConditionSequence::OnTimeElapsed(float ElapsedTime)
{
	if (CurrChild >= Children.size()) return 0;

	if (Children[CurrChild]->OnTimeElapsed(ElapsedTime))
	{
		if (++CurrChild == Children.size())
		{
			CurrChild = 0;
			return 1;
		}
	}

	// Sequence timeout can be added, then reset will be performed here

	return 0;
}
//---------------------------------------------------------------------

}
