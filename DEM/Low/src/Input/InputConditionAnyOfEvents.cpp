#include "InputConditionAnyOfEvents.h"
#include <Input/InputEvents.h>
#include <Input/InputDevice.h>
#include <Input/InputConditionUp.h>

namespace Input
{
RTTI_CLASS_IMPL(Input::CInputConditionAnyOfEvents, Input::CInputConditionEvent);

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

bool CInputConditionAnyOfEvents::OnAxisMove(const IInputDevice* pDevice, const Event::AxisMove& Event)
{
	for (auto& Child : Children)
		if (Child->OnAxisMove(pDevice, Event)) OK;
	FAIL;
}
//---------------------------------------------------------------------

bool CInputConditionAnyOfEvents::OnButtonDown(const IInputDevice* pDevice, const Event::ButtonDown& Event)
{
	for (auto& Child : Children)
		if (Child->OnButtonDown(pDevice, Event)) OK;
	FAIL;
}
//---------------------------------------------------------------------

bool CInputConditionAnyOfEvents::OnButtonUp(const IInputDevice* pDevice, const Event::ButtonUp& Event)
{
	for (auto& Child : Children)
		if (Child->OnButtonUp(pDevice, Event)) OK;
	FAIL;
}
//---------------------------------------------------------------------

bool CInputConditionAnyOfEvents::OnTextInput(const IInputDevice* pDevice, const Event::TextInput& Event)
{
	for (auto& Child : Children)
		if (Child->OnTextInput(pDevice, Event)) OK;
	FAIL;
}
//---------------------------------------------------------------------

bool CInputConditionAnyOfEvents::OnTimeElapsed(float ElapsedTime)
{
	for (auto& Child : Children)
		if (Child->OnTimeElapsed(ElapsedTime)) OK;
	FAIL;
}
//---------------------------------------------------------------------

}