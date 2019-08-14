#include "InputConditionAnyOfEvents.h"
#include <Input/InputEvents.h>
#include <Input/InputDevice.h>
#include <Input/InputConditionUp.h>

namespace Input
{
__ImplementClassNoFactory(Input::CInputConditionAnyOfEvents, Input::CInputConditionEvent);

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