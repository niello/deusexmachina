#pragma once
#include <Input/InputConditionEvent.h>
#include <Input/InputConditionState.h>

// Event condition that is triggered when the child state is on and the child event is triggered.
// Useful for combinations like "[Ctrl + Shift] + Click" or "[MMB] + Mouse move".

namespace Input
{

class CInputConditionComboEvent: public CInputConditionEvent
{
	RTTI_CLASS_DECL(Input::CInputConditionComboEvent, Input::CInputConditionEvent);

protected:

	PInputConditionEvent _Event;
	PInputConditionState _State;

	void         Clear();

public:

	void         AddChild(PInputConditionEvent&& NewChild);
	void         AddChild(PInputConditionState&& NewChild);

	virtual bool UpdateParams(std::function<std::string(const char*)> ParamGetter, std::set<std::string>* pOutParams) override;
	virtual void Reset() override;
	virtual UPTR OnAxisMove(const IInputDevice* pDevice, const Event::AxisMove& Event) override;
	virtual UPTR OnButtonDown(const IInputDevice* pDevice, const Event::ButtonDown& Event) override;
	virtual UPTR OnButtonUp(const IInputDevice* pDevice, const Event::ButtonUp& Event) override;
	virtual UPTR OnTextInput(const IInputDevice* pDevice, const Event::TextInput& Event) override;
	virtual UPTR OnTimeElapsed(float ElapsedTime) override;
};

}
