#include "InputConditionUp.h"

#include <Input/InputEvents.h>
#include <Input/InputDevice.h>
#include <Core/Factory.h>

namespace Input
{
__ImplementClass(Input::CInputConditionUp, 'ICUP', Input::CInputConditionEvent);

bool CInputConditionUp::Initialize(const Data::CParams& Desc)
{
	DeviceType = StringToDeviceType(Desc.Get<CString>(CStrID("Device"), CString::Empty));

	const Data::CData& ButtonData = Desc.Get(CStrID("Button")).GetRawValue();
	if (ButtonData.IsA<int>()) Button = (U8)ButtonData.GetValue<int>();
	else if (ButtonData.IsA<CString>()) Button = StringToButton(DeviceType, ButtonData.GetValue<CString>());

	OK;
}
//---------------------------------------------------------------------

bool CInputConditionUp::OnButtonUp(const IInputDevice* pDevice, const Event::ButtonUp& Event)
{
	return pDevice->GetType() == DeviceType && Event.Code == Button;
}
//---------------------------------------------------------------------

}