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

	const Data::CData& ButtonData = Desc.Get(CStrID("Button")).GetRawValue();
	if (ButtonData.IsA<int>()) Button = (U8)ButtonData.GetValue<int>();
	else if (ButtonData.IsA<CString>()) Button = StringToButton(DeviceType, ButtonData.GetValue<CString>());

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