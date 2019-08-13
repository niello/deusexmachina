#include "InputConditionPressed.h"
#include <Input/InputEvents.h>

namespace Input
{

CInputConditionPressed::CInputConditionPressed(EDeviceType DeviceType, U8 Button)
	: _DeviceType(DeviceType)
	, _Button(Button)
{
}
//---------------------------------------------------------------------

void CInputConditionPressed::OnButtonDown(const IInputDevice* pDevice, const Event::ButtonDown& Event)
{
	if (pDevice->GetType() == _DeviceType && Event.Code == _Button) On = true;
}
//---------------------------------------------------------------------

void CInputConditionPressed::OnButtonUp(const IInputDevice* pDevice, const Event::ButtonUp& Event)
{
	if (pDevice->GetType() == _DeviceType && Event.Code == _Button) On = false;
}
//---------------------------------------------------------------------

}