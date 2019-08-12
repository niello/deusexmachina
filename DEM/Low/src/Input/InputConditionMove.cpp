#include "InputConditionMove.h"
#include <Input/InputEvents.h>
#include <Input/InputDevice.h>
#include <Core/Factory.h>

namespace Input
{
__ImplementClass(Input::CInputConditionMove, 'ICMV', Input::CInputConditionEvent);

bool CInputConditionMove::Initialize(const Data::CParams& Desc)
{
	DeviceType = StringToDeviceType(Desc.Get<CString>(CStrID("Device"), CString::Empty));
	Axis = StringToMouseAxis(Desc.Get<CString>(CStrID("Axis"), CString::Empty));
	OK;
}
//---------------------------------------------------------------------

bool CInputConditionMove::OnAxisMove(const IInputDevice* pDevice, const Event::AxisMove& Event)
{
	if (pDevice->GetType() != DeviceType || Event.Code != Axis) FAIL;

	if (Threshold <= 0.f) OK;

	Accumulated += Event.Amount;
	if (Accumulated < Threshold) FAIL;

	Accumulated -= Threshold;
	OK;
}
//---------------------------------------------------------------------

}