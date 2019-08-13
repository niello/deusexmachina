#include "InputConditionReleased.h"
#include <Input/InputEvents.h>

namespace Input
{

CInputConditionReleased::CInputConditionReleased(EDeviceType DeviceType, U8 Button)
	: _DeviceType(DeviceType)
	, _Button(Button)
{
}
//---------------------------------------------------------------------

void CInputConditionReleased::OnButtonDown(const IInputDevice* pDevice, const Event::ButtonDown& Event)
{
	if (pDevice->GetType() == _DeviceType && Event.Code == _Button) On = false;
}
//---------------------------------------------------------------------

void CInputConditionReleased::OnButtonUp(const IInputDevice* pDevice, const Event::ButtonUp& Event)
{
	if (pDevice->GetType() == _DeviceType && Event.Code == _Button) On = true;
}
//---------------------------------------------------------------------

}