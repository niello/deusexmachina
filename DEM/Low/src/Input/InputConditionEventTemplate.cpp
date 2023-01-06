#include "InputConditionEventTemplate.h"
#include <cctype>

namespace Input
{

CInputConditionEventTemplate::CInputConditionEventTemplate(const std::string& RuleTemplate)
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

bool CInputConditionEventTemplate::UpdateParams(std::function<std::string(const char*)> ParamGetter, std::set<std::string>* pOutParams)
{
	if (!ParamGetter || _ParamStart == std::string::npos) FAIL;

	if (pOutParams) pOutParams->insert(_Key);

	const std::string NewValue = ParamGetter(_Key.c_str());
	if (_Impl && NewValue == _CurrValue) OK;

	_Impl.reset();

	// There is exactly one parameter in a rule by design
	auto RuleStr = _RuleTemplate;
	RuleStr.replace(_ParamStart, _Key.size() + 1, NewValue.c_str());

	auto pNewImpl = ParseRule(RuleStr.c_str());
	if (!pNewImpl) FAIL;

	_Impl.reset(pNewImpl->As<CInputConditionEvent>());
	return _Impl != nullptr;
}
//---------------------------------------------------------------------

void CInputConditionEventTemplate::Reset()
{
	if (_Impl) _Impl->Reset();
}
//---------------------------------------------------------------------

UPTR CInputConditionEventTemplate::OnAxisMove(const IInputDevice* pDevice, const Event::AxisMove& Event)
{
	return _Impl ? _Impl->OnAxisMove(pDevice, Event) : 0;
}
//---------------------------------------------------------------------

UPTR CInputConditionEventTemplate::OnButtonDown(const IInputDevice* pDevice, const Event::ButtonDown& Event)
{
	return _Impl ? _Impl->OnButtonDown(pDevice, Event) : 0;
}
//---------------------------------------------------------------------

UPTR CInputConditionEventTemplate::OnButtonUp(const IInputDevice* pDevice, const Event::ButtonUp& Event)
{
	return _Impl ? _Impl->OnButtonUp(pDevice, Event) : 0;
}
//---------------------------------------------------------------------

UPTR CInputConditionEventTemplate::OnTextInput(const IInputDevice* pDevice, const Event::TextInput& Event)
{
	return _Impl ? _Impl->OnTextInput(pDevice, Event) : 0;
}
//---------------------------------------------------------------------

UPTR CInputConditionEventTemplate::OnTimeElapsed(float ElapsedTime)
{
	return _Impl ? _Impl->OnTimeElapsed(ElapsedTime) : 0;
}
//---------------------------------------------------------------------

}
