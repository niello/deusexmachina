#include "InputConditionDown.h"
#include <Input/InputEvents.h>

namespace Input
{

CInputConditionDown::CInputConditionDown(EDeviceType DeviceType, U8 Button)
	: _DeviceType(DeviceType)
	, _Button(Button)
{
}
//---------------------------------------------------------------------

bool CInputConditionDown::OnButtonDown(const IInputDevice* pDevice, const Event::ButtonDown& Event)
{
	return pDevice->GetType() == _DeviceType && Event.Code == _Button;
}
//---------------------------------------------------------------------

}