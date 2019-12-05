#include "InputConditionAnyOfStates.h"

namespace Input
{
RTTI_CLASS_IMPL(Input::CInputConditionAnyOfStates, Input::CInputConditionState);

void CInputConditionAnyOfStates::AddChild(PInputConditionState&& NewChild)
{
	if (!NewChild || NewChild.get() == this) return;
	Children.push_back(std::move(NewChild));
}
//---------------------------------------------------------------------

void CInputConditionAnyOfStates::Clear()
{
	Children.clear();
	On = false;
}
//---------------------------------------------------------------------

bool CInputConditionAnyOfStates::UpdateParams(std::function<std::string(const char*)> ParamGetter, std::set<std::string>* pOutParams)
{
	bool Result = true;
	for (auto& Child : Children)
		Result &= Child->UpdateParams(ParamGetter, pOutParams);
	return Result;
}
//---------------------------------------------------------------------

void CInputConditionAnyOfStates::Reset()
{
	On = false;
	for (auto& Child : Children)
	{
		Child->Reset();
		if (Child->IsOn())
		{
			On = true;
			break;
		}
	}
}
//---------------------------------------------------------------------

void CInputConditionAnyOfStates::OnAxisMove(const IInputDevice* pDevice, const Event::AxisMove& Event)
{
	On = false;
	for (auto& Child : Children)
	{
		Child->OnAxisMove(pDevice, Event);
		if (Child->IsOn())
		{
			On = true;
			break;
		}
	}
}
//---------------------------------------------------------------------

void CInputConditionAnyOfStates::OnButtonDown(const IInputDevice* pDevice, const Event::ButtonDown& Event)
{
	On = false;
	for (auto& Child : Children)
	{
		Child->OnButtonDown(pDevice, Event);
		if (Child->IsOn())
		{
			On = true;
			break;
		}
	}
}
//---------------------------------------------------------------------

void CInputConditionAnyOfStates::OnButtonUp(const IInputDevice* pDevice, const Event::ButtonUp& Event)
{
	On = false;
	for (auto& Child : Children)
	{
		Child->OnButtonUp(pDevice, Event);
		if (Child->IsOn())
		{
			On = true;
			break;
		}
	}
}
//---------------------------------------------------------------------

void CInputConditionAnyOfStates::OnTextInput(const IInputDevice* pDevice, const Event::TextInput& Event)
{
	On = false;
	for (auto& Child : Children)
	{
		Child->OnTextInput(pDevice, Event);
		if (Child->IsOn())
		{
			On = true;
			break;
		}
	}
}
//---------------------------------------------------------------------

void CInputConditionAnyOfStates::OnTimeElapsed(float ElapsedTime)
{
	On = false;
	for (auto& Child : Children)
	{
		Child->OnTimeElapsed(ElapsedTime);
		if (Child->IsOn())
		{
			On = true;
			break;
		}
	}
}
//---------------------------------------------------------------------

}