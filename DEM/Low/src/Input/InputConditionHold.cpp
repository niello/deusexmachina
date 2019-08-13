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

bool CInputConditionHold::OnButtonDown(const IInputDevice* pDevice, const Event::ButtonDown& Event)
{
	if (pDevice->GetType() == _DeviceType && Event.Code == _Button)
	{
		_Waiting = true;
		_TimeSinceDown = 0.f;
	}
	FAIL;
}
//---------------------------------------------------------------------

bool CInputConditionHold::OnButtonUp(const IInputDevice* pDevice, const Event::ButtonUp& Event)
{
	if (pDevice->GetType() == _DeviceType && Event.Code == _Button)
		_Waiting = false;
	FAIL;
}
//---------------------------------------------------------------------

bool CInputConditionHold::OnTimeElapsed(float ElapsedTime)
{
	if (!_Waiting) FAIL;
	_TimeSinceDown += ElapsedTime;
	if (_TimeSinceDown < _Time) FAIL;
	if (_Repeat) _TimeSinceDown -= _Time;
	else _Waiting = false;
	OK;
}
//---------------------------------------------------------------------

}