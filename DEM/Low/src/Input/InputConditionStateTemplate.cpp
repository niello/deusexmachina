#include "InputConditionStateTemplate.h"
#include <cctype>

namespace Input
{
__ImplementClassNoFactory(Input::CInputConditionStateTemplate, Input::CInputConditionState);

CInputConditionStateTemplate::CInputConditionStateTemplate(const std::string& RuleTemplate)
	: _RuleTemplate(RuleTemplate)
	, _ParamStart(RuleTemplate.find('$'))
{
	auto pKeyStart = _RuleTemplate.c_str() + _ParamStart + 1;
	auto pKeyEnd = pKeyStart;
	while (*pKeyEnd == '_' || std::isalnum(static_cast<unsigned char>(*pKeyEnd)))
		++pKeyEnd;
	_Key.assign(pKeyStart, pKeyEnd);
}
//---------------------------------------------------------------------

bool CInputConditionStateTemplate::UpdateParams(std::function<std::string(const char*)> ParamGetter)
{
	if (!ParamGetter || _ParamStart == std::string::npos) FAIL;

	const std::string NewValue = ParamGetter(_Key.c_str());
	if (_Impl && NewValue == _CurrValue) OK;

	_Impl.reset();

	// There is exactly one parameter in a rule by design
	auto RuleStr = _RuleTemplate;
	RuleStr.replace(_ParamStart, _Key.size() + 1, NewValue.c_str());

	auto pNewImpl = ParseRule(RuleStr.c_str());
	if (!pNewImpl) FAIL;

	_Impl.reset(pNewImpl->As<CInputConditionState>());
	if (!_Impl) FAIL;

	On = _Impl->IsOn();
	OK;
}
//---------------------------------------------------------------------

void CInputConditionStateTemplate::Reset()
{
	if (_Impl) _Impl->Reset();
}
//---------------------------------------------------------------------

void CInputConditionStateTemplate::OnAxisMove(const IInputDevice* pDevice, const Event::AxisMove& Event)
{
	if (_Impl)
	{
		_Impl->OnAxisMove(pDevice, Event);
		On = _Impl->IsOn();
	}
	else On = false;
}
//---------------------------------------------------------------------

void CInputConditionStateTemplate::OnButtonDown(const IInputDevice* pDevice, const Event::ButtonDown& Event)
{
	if (_Impl)
	{
		_Impl->OnButtonDown(pDevice, Event);
		On = _Impl->IsOn();
	}
	else On = false;
}
//---------------------------------------------------------------------

void CInputConditionStateTemplate::OnButtonUp(const IInputDevice* pDevice, const Event::ButtonUp& Event)
{
	if (_Impl)
	{
		_Impl->OnButtonUp(pDevice, Event);
		On = _Impl->IsOn();
	}
	else On = false;
}
//---------------------------------------------------------------------

void CInputConditionStateTemplate::OnTextInput(const IInputDevice* pDevice, const Event::TextInput& Event)
{
	if (_Impl)
	{
		_Impl->OnTextInput(pDevice, Event);
		On = _Impl->IsOn();
	}
	else On = false;
}
//---------------------------------------------------------------------

void CInputConditionStateTemplate::OnTimeElapsed(float ElapsedTime)
{
	if (_Impl)
	{
		_Impl->OnTimeElapsed(ElapsedTime);
		On = _Impl->IsOn();
	}
	else On = false;
}
//---------------------------------------------------------------------

}