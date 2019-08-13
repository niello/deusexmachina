#include "InputConditionComboEvent.h"
#include <Data/Params.h>
#include <Core/Factory.h>

namespace Input
{
__ImplementClass(Input::CInputConditionComboEvent, 'ICCE', Input::CInputConditionEvent);

void CInputConditionComboEvent::Clear()
{
	SAFE_DELETE(pEvent);
	SAFE_DELETE(pState);
}
//---------------------------------------------------------------------

void CInputConditionComboEvent::Reset()
{
	if (pEvent) pEvent->Reset();
	if (pState) pState->Reset();
}
//---------------------------------------------------------------------

bool CInputConditionComboEvent::OnAxisMove(const IInputDevice* pDevice, const Event::AxisMove& Event)
{
	if (pState)
	{
		pState->OnAxisMove(pDevice, Event);
		if (!pState->IsOn()) FAIL;
	}
	return pEvent->OnAxisMove(pDevice, Event);
}
//---------------------------------------------------------------------

bool CInputConditionComboEvent::OnButtonDown(const IInputDevice* pDevice, const Event::ButtonDown& Event)
{
	if (pState)
	{
		pState->OnButtonDown(pDevice, Event);
		if (!pState->IsOn()) FAIL;
	}
	return pEvent->OnButtonDown(pDevice, Event);
}
//---------------------------------------------------------------------

bool CInputConditionComboEvent::OnButtonUp(const IInputDevice* pDevice, const Event::ButtonUp& Event)
{
	if (pState)
	{
		pState->OnButtonUp(pDevice, Event);
		if (!pState->IsOn()) FAIL;
	}
	return pEvent->OnButtonUp(pDevice, Event);
}
//---------------------------------------------------------------------

bool CInputConditionComboEvent::OnTextInput(const IInputDevice* pDevice, const Event::TextInput& Event)
{
	if (pState)
	{
		pState->OnTextInput(pDevice, Event);
		if (!pState->IsOn()) FAIL;
	}
	return pEvent->OnTextInput(pDevice, Event);
}
//---------------------------------------------------------------------

bool CInputConditionComboEvent::OnTimeElapsed(float ElapsedTime)
{
	if (pState)
	{
		pState->OnTimeElapsed(ElapsedTime);
		if (!pState->IsOn()) FAIL;
	}
	return pEvent->OnTimeElapsed(ElapsedTime);
}
//---------------------------------------------------------------------

}