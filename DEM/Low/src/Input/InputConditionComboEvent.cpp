#include "InputConditionComboEvent.h"
#include <Input/InputConditionSequence.h>
#include <Input/InputConditionComboState.h>

namespace Input
{

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
			_Event.reset(pSequence);
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
			_State.reset(pCombo);
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

UPTR CInputConditionComboEvent::OnAxisMove(const IInputDevice* pDevice, const Event::AxisMove& Event)
{
	if (_State)
	{
		_State->OnAxisMove(pDevice, Event);
		if (!_State->IsOn()) return 0;
	}
	return _Event->OnAxisMove(pDevice, Event);
}
//---------------------------------------------------------------------

UPTR CInputConditionComboEvent::OnButtonDown(const IInputDevice* pDevice, const Event::ButtonDown& Event)
{
	if (_State)
	{
		_State->OnButtonDown(pDevice, Event);
		if (!_State->IsOn()) return 0;
	}
	return _Event->OnButtonDown(pDevice, Event);
}
//---------------------------------------------------------------------

UPTR CInputConditionComboEvent::OnButtonUp(const IInputDevice* pDevice, const Event::ButtonUp& Event)
{
	if (_State)
	{
		_State->OnButtonUp(pDevice, Event);
		if (!_State->IsOn()) return 0;
	}
	return _Event->OnButtonUp(pDevice, Event);
}
//---------------------------------------------------------------------

UPTR CInputConditionComboEvent::OnTextInput(const IInputDevice* pDevice, const Event::TextInput& Event)
{
	if (_State)
	{
		_State->OnTextInput(pDevice, Event);
		if (!_State->IsOn()) return 0;
	}
	return _Event->OnTextInput(pDevice, Event);
}
//---------------------------------------------------------------------

UPTR CInputConditionComboEvent::OnTimeElapsed(float ElapsedTime)
{
	if (_State)
	{
		_State->OnTimeElapsed(ElapsedTime);
		if (!_State->IsOn()) return 0;
	}
	return _Event->OnTimeElapsed(ElapsedTime);
}
//---------------------------------------------------------------------

}
