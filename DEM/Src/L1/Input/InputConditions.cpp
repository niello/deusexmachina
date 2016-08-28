#include "InputConditions.h"

#include <Input/InputEvents.h>
#include <Input/InputDevice.h>

namespace Input
{

bool CInputConditionMove::Initialize(const Data::CParams& Desc)
{
	FAIL;
}
//---------------------------------------------------------------------

bool CInputConditionMove::OnAxisMove(const IInputDevice* pDevice, const Event::AxisMove& Event, bool& Triggered) const
{
	Triggered = (pDevice->GetType() == DeviceType && Event.Code == Axis);
	OK;
}
//---------------------------------------------------------------------

bool CInputConditionDown::Initialize(const Data::CParams& Desc)
{
	FAIL;
}
//---------------------------------------------------------------------

bool CInputConditionDown::OnButtonDown(const IInputDevice* pDevice, const Event::ButtonDown& Event, bool& Triggered) const
{
	Triggered = (pDevice->GetType() == DeviceType && Event.Code == Button);
	OK;
}
//---------------------------------------------------------------------

bool CInputConditionUp::Initialize(const Data::CParams& Desc)
{
	FAIL;
}
//---------------------------------------------------------------------

bool CInputConditionUp::OnButtonUp(const IInputDevice* pDevice, const Event::ButtonUp& Event, bool& Triggered) const
{
	Triggered = (pDevice->GetType() == DeviceType && Event.Code == Button);
	OK;
}
//---------------------------------------------------------------------

}