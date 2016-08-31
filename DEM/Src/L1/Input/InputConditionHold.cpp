#include "InputConditionHold.h"

#include <Input/InputEvents.h>
#include <Input/InputDevice.h>
#include <Core/Factory.h>

namespace Input
{
__ImplementClass(Input::CInputConditionHold, 'ICHL', Input::CInputConditionEvent);

bool CInputConditionHold::Initialize(const Data::CParams& Desc)
{
	DeviceType = StringToDeviceType(Desc.Get<CString>(CStrID("Device"), CString::Empty));

	const Data::CData& ButtonData = Desc.Get(CStrID("Button")).GetRawValue();
	if (ButtonData.IsA<int>()) Button = (U8)ButtonData.GetValue<int>();
	else if (ButtonData.IsA<CString>()) Button = StringToButton(DeviceType, ButtonData.GetValue<CString>());

	Time = Desc.Get<float>(CStrID("Time"), 0.f);
	Repeat = Desc.Get<bool>(CStrID("Repeat"), false);

	Waiting = false;

	OK;
}
//---------------------------------------------------------------------

bool CInputConditionHold::OnButtonDown(const IInputDevice* pDevice, const Event::ButtonDown& Event)
{
	if (pDevice->GetType() == DeviceType && Event.Code == Button)
	{
		Waiting = true;
		TimeSinceDown = 0.f;
	}
	FAIL;
}
//---------------------------------------------------------------------

bool CInputConditionHold::OnButtonUp(const IInputDevice* pDevice, const Event::ButtonUp& Event)
{
	if (pDevice->GetType() == DeviceType && Event.Code == Button)
		Waiting = false;
	FAIL;
}
//---------------------------------------------------------------------

bool CInputConditionHold::OnTimeElapsed(float ElapsedTime)
{
	if (!Waiting) FAIL;
	TimeSinceDown += ElapsedTime;
	if (TimeSinceDown < Time) FAIL;
	if (Repeat) TimeSinceDown = 0.f;
	else Waiting = false;
	OK;
}
//---------------------------------------------------------------------

}