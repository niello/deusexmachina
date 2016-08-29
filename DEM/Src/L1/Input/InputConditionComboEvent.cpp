#include "InputConditionComboEvent.h"

#include <Input/InputEvents.h>
#include <Input/InputDevice.h>

namespace Input
{

bool CInputConditionComboEvent::Initialize(const Data::CParams& Desc)
{
	SAFE_DELETE(pEvent);
	SAFE_DELETE(pState);

	Data::PParams SubDesc;
	if (Desc.Get<Data::PParams>(SubDesc, CStrID("Event")))
	{
		pEvent = CInputConditionEvent::CreateByType(SubDesc->Get<CString>(CStrID("Type")));
		if (!pEvent || !pEvent->Initialize(*SubDesc.GetUnsafe())) FAIL;
	}

	if (Desc.Get<Data::PParams>(SubDesc, CStrID("State")))
	{
		pState = CInputConditionState::CreateByType(SubDesc->Get<CString>(CStrID("Type")));
		if (!pState || !pState->Initialize(*SubDesc.GetUnsafe())) FAIL;
	}

	OK;
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
	if (pState && !pState->IsOn()) FAIL;
	return pEvent->OnAxisMove(pDevice, Event);
}
//---------------------------------------------------------------------

bool CInputConditionComboEvent::OnButtonDown(const IInputDevice* pDevice, const Event::ButtonDown& Event)
{
	if (pState && !pState->IsOn()) FAIL;
	return pEvent->OnButtonDown(pDevice, Event);
}
//---------------------------------------------------------------------

bool CInputConditionComboEvent::OnButtonUp(const IInputDevice* pDevice, const Event::ButtonUp& Event)
{
	if (pState && !pState->IsOn()) FAIL;
	return pEvent->OnButtonUp(pDevice, Event);
}
//---------------------------------------------------------------------

bool CInputConditionComboEvent::OnTimeElapsed(float ElapsedTime)
{
	if (pState && !pState->IsOn()) FAIL;
	return pEvent->OnTimeElapsed(ElapsedTime);
}
//---------------------------------------------------------------------

}