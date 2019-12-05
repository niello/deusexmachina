#include "InputConditionComboEvent.h"
#include <Input/InputConditionSequence.h>
#include <Input/InputConditionComboState.h>

namespace Input
{
RTTI_CLASS_IMPL(Input::CInputConditionComboEvent, Input::CInputConditionEvent);

void CInputConditionComboEvent::AddChild(PInputConditionEvent&& NewChild)
{
	if (!NewChild || NewChild.get() == this || NewChild == _Event) return;

	if (_Event)
	{
		// Automatically combine
		auto pSequence = _Event->As<CInputConditionSequence>();
		if (!pSequence)
		{
			pSequence = new CInputConditionSequence();
			pSequence->AddChild(std::move(_Event));
		}
		pSequence->AddChild(std::move(NewChild));
	}
	else _Event = std::move(NewChild);
}
//---------------------------------------------------------------------

void CInputConditionComboEvent::AddChild(PInputConditionState&& NewChild)
{
	if (!NewChild || NewChild == _State) return;

	if (_State)
	{
		// Automatically combine
		auto pCombo = _State->As<CInputConditionComboState>();
		if (!pCombo)
		{
			pCombo = new CInputConditionComboState();
			pCombo->AddChild(std::move(_State));
		}
		pCombo->AddChild(std::move(NewChild));
	}
	else _State = std::move(NewChild);
}
//---------------------------------------------------------------------

void CInputConditionComboEvent::Clear()
{
	_Event.reset();
	_State.reset();
}
//---------------------------------------------------------------------

bool CInputConditionComboEvent::UpdateParams(std::function<std::string(const char*)> ParamGetter, std::set<std::string>* pOutParams)
{
	bool Result = true;
	if (_Event) Result &= _Event->UpdateParams(ParamGetter, pOutParams);
	if (_State) Result &= _State->UpdateParams(ParamGetter, pOutParams);
	return Result;
}
//---------------------------------------------------------------------

void CInputConditionComboEvent::Reset()
{
	if (_Event) _Event->Reset();
	if (_State) _State->Reset();
}
//---------------------------------------------------------------------

bool CInputConditionComboEvent::OnAxisMove(const IInputDevice* pDevice, const Event::AxisMove& Event)
{
	if (_State)
	{
		_State->OnAxisMove(pDevice, Event);
		if (!_State->IsOn()) FAIL;
	}
	return _Event->OnAxisMove(pDevice, Event);
}
//---------------------------------------------------------------------

bool CInputConditionComboEvent::OnButtonDown(const IInputDevice* pDevice, const Event::ButtonDown& Event)
{
	if (_State)
	{
		_State->OnButtonDown(pDevice, Event);
		if (!_State->IsOn()) FAIL;
	}
	return _Event->OnButtonDown(pDevice, Event);
}
//---------------------------------------------------------------------

bool CInputConditionComboEvent::OnButtonUp(const IInputDevice* pDevice, const Event::ButtonUp& Event)
{
	if (_State)
	{
		_State->OnButtonUp(pDevice, Event);
		if (!_State->IsOn()) FAIL;
	}
	return _Event->OnButtonUp(pDevice, Event);
}
//---------------------------------------------------------------------

bool CInputConditionComboEvent::OnTextInput(const IInputDevice* pDevice, const Event::TextInput& Event)
{
	if (_State)
	{
		_State->OnTextInput(pDevice, Event);
		if (!_State->IsOn()) FAIL;
	}
	return _Event->OnTextInput(pDevice, Event);
}
//---------------------------------------------------------------------

bool CInputConditionComboEvent::OnTimeElapsed(float ElapsedTime)
{
	if (_State)
	{
		_State->OnTimeElapsed(ElapsedTime);
		if (!_State->IsOn()) FAIL;
	}
	return _Event->OnTimeElapsed(ElapsedTime);
}
//---------------------------------------------------------------------

}