#include "InputConditionUp.h"

#include <Input/InputEvents.h>
#include <Input/InputDevice.h>

namespace Input
{

bool CInputConditionUp::Initialize(const Data::CParams& Desc)
{
	DeviceType = StringToDeviceType(Desc.Get<CString>(CStrID("Device"), CString::Empty));
	Button = StringToMouseButton(Desc.Get<CString>(CStrID("Button"), CString::Empty));
	OK;
}
//---------------------------------------------------------------------

bool CInputConditionUp::OnButtonUp(const IInputDevice* pDevice, const Event::ButtonUp& Event)
{
	return pDevice->GetType() == DeviceType && Event.Code == Button;
}
//---------------------------------------------------------------------

}