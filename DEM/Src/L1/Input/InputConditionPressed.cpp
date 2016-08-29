#include "InputConditionPressed.h"

#include <Input/InputEvents.h>
#include <Input/InputDevice.h>
#include <Core/Factory.h>

namespace Input
{
__ImplementClass(Input::CInputConditionPressed, 'ICPR', Input::CInputConditionState);

bool CInputConditionPressed::Initialize(const Data::CParams& Desc)
{
	On = false;
	DeviceType = StringToDeviceType(Desc.Get<CString>(CStrID("Device"), CString::Empty));
	Button = StringToMouseButton(Desc.Get<CString>(CStrID("Button"), CString::Empty));
	OK;
}
//---------------------------------------------------------------------

void CInputConditionPressed::OnButtonDown(const IInputDevice* pDevice, const Event::ButtonDown& Event)
{
	if (pDevice->GetType() == DeviceType && Event.Code == Button) On = true;
}
//---------------------------------------------------------------------

void CInputConditionPressed::OnButtonUp(const IInputDevice* pDevice, const Event::ButtonUp& Event)
{
	if (pDevice->GetType() == DeviceType && Event.Code == Button) On = false;
}
//---------------------------------------------------------------------

}