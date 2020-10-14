#pragma once
#include <Input/InputConditionState.h>

// State condition that is On when any of its child conditions are On

namespace Input
{

class CInputConditionAnyOfStates: public CInputConditionState
{
	RTTI_CLASS_DECL(Input::CInputConditionAnyOfStates, Input::CInputConditionState);

protected:

	std::vector<PInputConditionState>	Children;

	void			Clear();

public:

	void			AddChild(PInputConditionState&& NewChild);

	virtual bool	UpdateParams(std::function<std::string(const char*)> ParamGetter, std::set<std::string>* pOutParams) override;
	virtual void	Reset() override;
	virtual void	OnAxisMove(const IInputDevice* pDevice, const Event::AxisMove& Event) override;
	virtual void	OnButtonDown(const IInputDevice* pDevice, const Event::ButtonDown& Event) override;
	virtual void	OnButtonUp(const IInputDevice* pDevice, const Event::ButtonUp& Event) override;
	virtual void	OnTextInput(const IInputDevice* pDevice, const Event::TextInput& Event) override;
	virtual void	OnTimeElapsed(float ElapsedTime) override;
};

}
