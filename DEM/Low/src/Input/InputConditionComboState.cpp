#include "InputConditionComboState.h"

#include <Input/InputEvents.h>
#include <Input/InputDevice.h>
#include <Data/DataArray.h>
#include <Core/Factory.h>

namespace Input
{
__ImplementClass(Input::CInputConditionComboState, 'ICCS', Input::CInputConditionEvent);

bool CInputConditionComboState::Initialize(const Data::CParams& Desc)
{
	Clear();

	Data::PDataArray StateDescArray;
	if (!Desc.Get<Data::PDataArray>(StateDescArray, CStrID("States"))) OK;

	Children.SetSize(StateDescArray->GetCount());
	for (UPTR i = 0; i < Children.GetCount(); ++i)
	{
		Data::PParams SubDesc = StateDescArray->Get<Data::PParams>(i);
		CInputConditionState* pState = CInputConditionState::CreateByType(SubDesc->Get<CString>(CStrID("Type")));
		if (!pState || !pState->Initialize(*SubDesc.Get())) FAIL;
		Children[i] = pState;
	}

	OK;
}
//---------------------------------------------------------------------

void CInputConditionComboState::Clear()
{
	for (UPTR i = 0; i < Children.GetCount(); ++i)
		if (Children[i]) n_delete(Children[i]);
	Children.SetSize(0);
	On = false;
}
//---------------------------------------------------------------------

void CInputConditionComboState::Reset()
{
	bool NewOn = true;
	for (UPTR i = 0; i < Children.GetCount(); ++i)
	{
		CInputConditionState* pState = Children[i];
		if (pState)
		{
			pState->Reset();
			if (NewOn && !pState->IsOn()) NewOn = false;
		}
	}
	On = NewOn;
}
//---------------------------------------------------------------------

void CInputConditionComboState::OnAxisMove(const IInputDevice* pDevice, const Event::AxisMove& Event)
{
	bool NewOn = true;
	for (UPTR i = 0; i < Children.GetCount(); ++i)
	{
		CInputConditionState* pState = Children[i];
		if (pState)
		{
			pState->OnAxisMove(pDevice, Event);
			if (NewOn && !pState->IsOn()) NewOn = false;
		}
	}
	On = NewOn;
}
//---------------------------------------------------------------------

void CInputConditionComboState::OnButtonDown(const IInputDevice* pDevice, const Event::ButtonDown& Event)
{
	bool NewOn = true;
	for (UPTR i = 0; i < Children.GetCount(); ++i)
	{
		CInputConditionState* pState = Children[i];
		if (pState)
		{
			pState->OnButtonDown(pDevice, Event);
			if (NewOn && !pState->IsOn()) NewOn = false;
		}
	}
	On = NewOn;
}
//---------------------------------------------------------------------

void CInputConditionComboState::OnButtonUp(const IInputDevice* pDevice, const Event::ButtonUp& Event)
{
	bool NewOn = true;
	for (UPTR i = 0; i < Children.GetCount(); ++i)
	{
		CInputConditionState* pState = Children[i];
		if (pState)
		{
			pState->OnButtonUp(pDevice, Event);
			if (NewOn && !pState->IsOn()) NewOn = false;
		}
	}
	On = NewOn;
}
//---------------------------------------------------------------------

void CInputConditionComboState::OnTimeElapsed(float ElapsedTime)
{
	bool NewOn = true;
	for (UPTR i = 0; i < Children.GetCount(); ++i)
	{
		CInputConditionState* pState = Children[i];
		if (pState)
		{
			pState->OnTimeElapsed(ElapsedTime);
			if (NewOn && !pState->IsOn()) NewOn = false;
		}
	}
	On = NewOn;
}
//---------------------------------------------------------------------

}