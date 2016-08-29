#include "InputConditionReleased.h"

#include <Input/InputEvents.h>
#include <Input/InputDevice.h>
#include <Core/Factory.h>

namespace Input
{
__ImplementClass(Input::CInputConditionReleased, 'ICRL', Input::CInputConditionState);

bool CInputConditionReleased::Initialize(const Data::CParams& Desc)
{
	On = true;
	DeviceType = StringToDeviceType(Desc.Get<CString>(CStrID("Device"), CString::Empty));
	Button = StringToMouseButton(Desc.Get<CString>(CStrID("Button"), CString::Empty));
	OK;
}
//---------------------------------------------------------------------

void CInputConditionReleased::OnButtonDown(const IInputDevice* pDevice, const Event::ButtonDown& Event)
{
	if (pDevice->GetType() == DeviceType && Event.Code == Button) On = false;
}
//---------------------------------------------------------------------

void CInputConditionReleased::OnButtonUp(const IInputDevice* pDevice, const Event::ButtonUp& Event)
{
	if (pDevice->GetType() == DeviceType && Event.Code == Button) On = true;
}
//---------------------------------------------------------------------

}