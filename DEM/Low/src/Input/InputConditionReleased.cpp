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

	const Data::CData& ButtonData = Desc.Get(CStrID("Button")).GetRawValue();
	if (ButtonData.IsA<int>()) Button = (U8)ButtonData.GetValue<int>();
	else if (ButtonData.IsA<CString>()) Button = StringToButton(DeviceType, ButtonData.GetValue<CString>());

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