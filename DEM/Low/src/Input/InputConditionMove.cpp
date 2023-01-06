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

UPTR CInputConditionMove::OnAxisMove(const IInputDevice* pDevice, const Event::AxisMove& Event)
{
	if (pDevice->GetType() != _DeviceType || Event.Code != _Axis) return 0;

	if (_Threshold <= 0.f) return 1;

	_Accumulated += Event.Amount;

	if (_Accumulated < _Threshold) return 0;

	const UPTR Count = static_cast<UPTR>(_Accumulated / _Threshold);
	_Accumulated -= Count * _Threshold;
	return Count;
}
//---------------------------------------------------------------------

}
