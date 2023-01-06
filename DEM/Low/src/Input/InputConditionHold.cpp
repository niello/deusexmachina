#include "InputConditionHold.h"
#include <Input/InputEvents.h>

namespace Input
{

CInputConditionHold::CInputConditionHold(EDeviceType DeviceType, U8 Button, float Time, bool Repeat)
	: _DeviceType(DeviceType)
	, _Button(Button)
	, _Time(Time)
	, _Repeat(Repeat)
{
}
//---------------------------------------------------------------------

UPTR CInputConditionHold::OnButtonDown(const IInputDevice* pDevice, const Event::ButtonDown& Event)
{
	if (pDevice->GetType() == _DeviceType && Event.Code == _Button)
	{
		_Waiting = true;
		_TimeSinceDown = 0.f;
	}
	return 0;
}
//---------------------------------------------------------------------

UPTR CInputConditionHold::OnButtonUp(const IInputDevice* pDevice, const Event::ButtonUp& Event)
{
	if (pDevice->GetType() == _DeviceType && Event.Code == _Button)
		_Waiting = false;
	return 0;
}
//---------------------------------------------------------------------

UPTR CInputConditionHold::OnTimeElapsed(float ElapsedTime)
{
	if (!_Waiting) return 0;

	_TimeSinceDown += ElapsedTime;

	if (_TimeSinceDown < _Time) return 0;

	if (_Repeat)
	{
		const UPTR Count = static_cast<UPTR>(_TimeSinceDown / _Time);
		_TimeSinceDown -= Count * _Time;
		return Count;
	}
	else
	{
		_Waiting = false;
		return 1;
	}
}
//---------------------------------------------------------------------

}
