#include "InputConditionComboState.h"

namespace Input
{
__ImplementClassNoFactory(Input::CInputConditionComboState, Input::CInputConditionState);

void CInputConditionComboState::AddChild(PInputConditionState&& NewChild)
{
	if (!NewChild || NewChild.get() == this) return;
	Children.push_back(std::move(NewChild));
}
//---------------------------------------------------------------------

void CInputConditionComboState::Clear()
{
	Children.clear();
	On = false;
}
//---------------------------------------------------------------------

bool CInputConditionComboState::UpdateParams(std::function<std::string(const char*)> ParamGetter, std::set<std::string>* pOutParams)
{
	bool Result = true;
	for (auto& Child : Children)
		Result &= Child->UpdateParams(ParamGetter, pOutParams);
	return Result;
}
//---------------------------------------------------------------------

void CInputConditionComboState::Reset()
{
	bool NewOn = true;
	for (auto& Child : Children)
	{
		Child->Reset();
		if (NewOn && !Child->IsOn()) NewOn = false;
	}
	On = NewOn;
}
//---------------------------------------------------------------------

void CInputConditionComboState::OnAxisMove(const IInputDevice* pDevice, const Event::AxisMove& Event)
{
	bool NewOn = true;
	for (auto& Child : Children)
	{
		Child->OnAxisMove(pDevice, Event);
		if (NewOn && !Child->IsOn()) NewOn = false;
	}
	On = NewOn;
}
//---------------------------------------------------------------------

void CInputConditionComboState::OnButtonDown(const IInputDevice* pDevice, const Event::ButtonDown& Event)
{
	bool NewOn = true;
	for (auto& Child : Children)
	{
		Child->OnButtonDown(pDevice, Event);
		if (NewOn && !Child->IsOn()) NewOn = false;
	}
	On = NewOn;
}
//---------------------------------------------------------------------

void CInputConditionComboState::OnButtonUp(const IInputDevice* pDevice, const Event::ButtonUp& Event)
{
	bool NewOn = true;
	for (auto& Child : Children)
	{
		Child->OnButtonUp(pDevice, Event);
		if (NewOn && !Child->IsOn()) NewOn = false;
	}
	On = NewOn;
}
//---------------------------------------------------------------------

void CInputConditionComboState::OnTextInput(const IInputDevice* pDevice, const Event::TextInput& Event)
{
	bool NewOn = true;
	for (auto& Child : Children)
	{
		Child->OnTextInput(pDevice, Event);
		if (NewOn && !Child->IsOn()) NewOn = false;
	}
	On = NewOn;
}
//---------------------------------------------------------------------

void CInputConditionComboState::OnTimeElapsed(float ElapsedTime)
{
	bool NewOn = true;
	for (auto& Child : Children)
	{
		Child->OnTimeElapsed(ElapsedTime);
		if (NewOn && !Child->IsOn()) NewOn = false;
	}
	On = NewOn;
}
//---------------------------------------------------------------------

}