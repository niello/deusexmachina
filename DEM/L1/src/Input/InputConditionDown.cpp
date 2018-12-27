#include "InputConditionDown.h"

#include <Input/InputEvents.h>
#include <Input/InputDevice.h>
#include <Core/Factory.h>

namespace Input
{
__ImplementClass(Input::CInputConditionDown, 'ICDN', Input::CInputConditionEvent);

bool CInputConditionDown::Initialize(const Data::CParams& Desc)
{
	DeviceType = StringToDeviceType(Desc.Get<CString>(CStrID("Device"), CString::Empty));

	const Data::CData& ButtonData = Desc.Get(CStrID("Button")).GetRawValue();
	if (ButtonData.IsA<int>()) Button = (U8)ButtonData.GetValue<int>();
	else if (ButtonData.IsA<CString>()) Button = StringToButton(DeviceType, ButtonData.GetValue<CString>());

	OK;
}
//---------------------------------------------------------------------

bool CInputConditionDown::OnButtonDown(const IInputDevice* pDevice, const Event::ButtonDown& Event)
{
	return pDevice->GetType() == DeviceType && Event.Code == Button;
}
//---------------------------------------------------------------------

}