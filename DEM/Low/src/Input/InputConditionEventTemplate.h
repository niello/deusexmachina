#pragma once
#include <Input/InputConditionEvent.h>

// Parametrized event condition helper

namespace Input
{

class CInputConditionEventTemplate: public CInputConditionEvent
{
	__DeclareClassNoFactory;

protected:

	std::string _RuleTemplate;
	std::string _Key;
	std::string _CurrValue;
	PInputConditionEvent _Impl;
	size_t _ParamStart;

public:

	CInputConditionEventTemplate(const std::string& RuleTemplate);

	virtual bool UpdateParams(std::function<std::string(const char*)> ParamGetter) override;
	virtual void Reset() override;
	virtual bool OnAxisMove(const IInputDevice* pDevice, const Event::AxisMove& Event) override;
	virtual bool OnButtonDown(const IInputDevice* pDevice, const Event::ButtonDown& Event) override;
	virtual bool OnButtonUp(const IInputDevice* pDevice, const Event::ButtonUp& Event) override;
	virtual bool OnTextInput(const IInputDevice* pDevice, const Event::TextInput& Event) override;
	virtual bool OnTimeElapsed(float ElapsedTime) override;
};

}
