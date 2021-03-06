#pragma once
#include <Input/InputConditionEvent.h>

// Event condition that is triggered when its child events are triggered in order.
// Condition is reset if sequence is broken by receiving any other input.

namespace Input
{

class CInputConditionSequence: public CInputConditionEvent
{
	RTTI_CLASS_DECL(Input::CInputConditionSequence, Input::CInputConditionEvent);

protected:

	std::vector<PInputConditionEvent>	Children;
	UPTR								CurrChild;

	void			Clear();

public:

	void			AddChild(PInputConditionEvent&& NewChild);

	virtual bool	UpdateParams(std::function<std::string(const char*)> ParamGetter, std::set<std::string>* pOutParams) override;
	virtual void	Reset() override;
	virtual bool	OnAxisMove(const IInputDevice* pDevice, const Event::AxisMove& Event) override;
	virtual bool	OnButtonDown(const IInputDevice* pDevice, const Event::ButtonDown& Event) override;
	virtual bool	OnButtonUp(const IInputDevice* pDevice, const Event::ButtonUp& Event) override;
	virtual bool	OnTextInput(const IInputDevice* pDevice, const Event::TextInput& Event) override;
	virtual bool	OnTimeElapsed(float ElapsedTime) override;
};

}
