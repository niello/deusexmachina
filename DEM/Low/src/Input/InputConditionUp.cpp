#include "InputConditionUp.h"
#include <Input/InputEvents.h>

namespace Input
{

CInputConditionUp::CInputConditionUp(EDeviceType DeviceType, U8 Button)
	: _DeviceType(DeviceType)
	, _Button(Button)
{
}
//---------------------------------------------------------------------

UPTR CInputConditionUp::OnButtonUp(const IInputDevice* pDevice, const Event::ButtonUp& Event)
{
	return (pDevice->GetType() == _DeviceType && Event.Code == _Button) ? 1 : 0;
}
//---------------------------------------------------------------------

}
