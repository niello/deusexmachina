#include "InputConditionAnyOfEvents.h"
#include <Input/InputEvents.h>
#include <Input/InputDevice.h>

namespace Input
{

void CInputConditionAnyOfEvents::AddChild(PInputConditionEvent&& NewChild)
{
	if (!NewChild || NewChild.get() == this) return;
	Children.push_back(std::move(NewChild));
}
//---------------------------------------------------------------------

void CInputConditionAnyOfEvents::Clear()
{
	Children.clear();
}
//---------------------------------------------------------------------

bool CInputConditionAnyOfEvents::UpdateParams(std::function<std::string(const char*)> ParamGetter, std::set<std::string>* pOutParams)
{
	bool Result = true;
	for (auto& Child : Children)
		Result &= Child->UpdateParams(ParamGetter, pOutParams);
	return Result;
}
//---------------------------------------------------------------------

void CInputConditionAnyOfEvents::Reset()
{
	for (auto& Child : Children)
		Child->Reset();
}
//---------------------------------------------------------------------

UPTR CInputConditionAnyOfEvents::OnAxisMove(const IInputDevice* pDevice, const Event::AxisMove& Event)
{
	UPTR Count = 0;
	for (auto& Child : Children)
		Count = std::max(Count, Child->OnAxisMove(pDevice, Event));
	return Count;
}
//---------------------------------------------------------------------

UPTR CInputConditionAnyOfEvents::OnButtonDown(const IInputDevice* pDevice, const Event::ButtonDown& Event)
{
	UPTR Count = 0;
	for (auto& Child : Children)
		Count = std::max(Count, Child->OnButtonDown(pDevice, Event));
	return Count;
}
//---------------------------------------------------------------------

UPTR CInputConditionAnyOfEvents::OnButtonUp(const IInputDevice* pDevice, const Event::ButtonUp& Event)
{
	UPTR Count = 0;
	for (auto& Child : Children)
		Count = std::max(Count, Child->OnButtonUp(pDevice, Event));
	return Count;
}
//---------------------------------------------------------------------

UPTR CInputConditionAnyOfEvents::OnTextInput(const IInputDevice* pDevice, const Event::TextInput& Event)
{
	UPTR Count = 0;
	for (auto& Child : Children)
		Count = std::max(Count, Child->OnTextInput(pDevice, Event));
	return Count;
}
//---------------------------------------------------------------------

UPTR CInputConditionAnyOfEvents::OnTimeElapsed(float ElapsedTime)
{
	UPTR Count = 0;
	for (auto& Child : Children)
		Count = std::max(Count, Child->OnTimeElapsed(ElapsedTime));
	return Count;
}
//---------------------------------------------------------------------

}
