#pragma once
#include <Input/InputConditionEvent.h>

// Parametrized event condition helper

namespace Input
{

class CInputConditionEventTemplate: public CInputConditionEvent
{
	RTTI_CLASS_DECL(Input::CInputConditionEventTemplate, Input::CInputConditionEvent);

protected:

	std::string _RuleTemplate;
	std::string _Key;
	std::string _CurrValue;
	PInputConditionEvent _Impl;
	size_t _ParamStart;

public:

	CInputConditionEventTemplate(const std::string& RuleTemplate);

	virtual bool UpdateParams(std::function<std::string(const char*)> ParamGetter, std::set<std::string>* pOutParams) override;
	virtual void Reset() override;
	virtual UPTR OnAxisMove(const IInputDevice* pDevice, const Event::AxisMove& Event) override;
	virtual UPTR OnButtonDown(const IInputDevice* pDevice, const Event::ButtonDown& Event) override;
	virtual UPTR OnButtonUp(const IInputDevice* pDevice, const Event::ButtonUp& Event) override;
	virtual UPTR OnTextInput(const IInputDevice* pDevice, const Event::TextInput& Event) override;
	virtual UPTR OnTimeElapsed(float ElapsedTime) override;
};

}
