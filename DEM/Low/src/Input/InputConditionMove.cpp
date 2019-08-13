#include "InputConditionMove.h"
#include <Input/InputEvents.h>

namespace Input
{

CInputConditionMove::CInputConditionMove(EDeviceType DeviceType, U8 Axis, float Threshold)
	: _DeviceType(DeviceType)
	, _Axis(Axis)
	, _Threshold(Threshold)
{
}
//---------------------------------------------------------------------

bool CInputConditionMove::OnAxisMove(const IInputDevice* pDevice, const Event::AxisMove& Event)
{
	if (pDevice->GetType() != _DeviceType || Event.Code != _Axis) FAIL;

	if (_Threshold <= 0.f) OK;

	_Accumulated += Event.Amount;
	if (_Accumulated < _Threshold) FAIL;

	_Accumulated -= _Threshold;
	OK;
}
//---------------------------------------------------------------------

}