#include "InputConditionComboState.h"
#include <Data/Params.h>
#include <Data/DataArray.h>
#include <Core/Factory.h>

namespace Input
{
__ImplementClass(Input::CInputConditionComboState, 'ICCS', Input::CInputConditionState);

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

void CInputConditionComboState::OnTextInput(const IInputDevice* pDevice, const Event::TextInput& Event)
{
	bool NewOn = true;
	for (UPTR i = 0; i < Children.GetCount(); ++i)
	{
		CInputConditionState* pState = Children[i];
		if (pState)
		{
			pState->OnTextInput(pDevice, Event);
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