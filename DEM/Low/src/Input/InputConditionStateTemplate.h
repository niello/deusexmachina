#pragma once
#include <Input/InputConditionState.h>

// Parametrized state condition helper

namespace Input
{

class CInputConditionStateTemplate: public CInputConditionState
{
	__DeclareClassNoFactory;

protected:

	std::string _RuleTemplate;
	std::string _Key;
	std::string _CurrValue;
	PInputConditionState _Impl;
	size_t _ParamStart;

public:

	CInputConditionStateTemplate(const std::string& RuleTemplate);

	virtual bool UpdateParams(std::function<std::string(const char*)> ParamGetter) override;
	virtual void Reset() override;
	virtual void OnAxisMove(const IInputDevice* pDevice, const Event::AxisMove& State) override;
	virtual void OnButtonDown(const IInputDevice* pDevice, const Event::ButtonDown& State) override;
	virtual void OnButtonUp(const IInputDevice* pDevice, const Event::ButtonUp& State) override;
	virtual void OnTextInput(const IInputDevice* pDevice, const Event::TextInput& State) override;
	virtual void OnTimeElapsed(float ElapsedTime) override;
};

}
