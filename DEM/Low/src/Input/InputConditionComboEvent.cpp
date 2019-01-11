#include "InputConditionComboEvent.h"

#include <Input/InputEvents.h>
#include <Input/InputDevice.h>
#include <Core/Factory.h>

namespace Input
{
__ImplementClass(Input::CInputConditionComboEvent, 'ICCE', Input::CInputConditionEvent);

bool CInputConditionComboEvent::Initialize(const Data::CParams& Desc)
{
	Clear();

	Data::PParams SubDesc;
	if (Desc.Get<Data::PParams>(SubDesc, CStrID("ChildEvent")))
	{
		pEvent = CInputConditionEvent::CreateByType(SubDesc->Get<CString>(CStrID("Type")));
		if (!pEvent || !pEvent->Initialize(*SubDesc.Get())) FAIL;
	}

	if (Desc.Get<Data::PParams>(SubDesc, CStrID("ChildState")))
	{
		pState = CInputConditionState::CreateByType(SubDesc->Get<CString>(CStrID("Type")));
		if (!pState || !pState->Initialize(*SubDesc.Get())) FAIL;
	}

	OK;
}
//---------------------------------------------------------------------

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