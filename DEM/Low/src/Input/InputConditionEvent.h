#pragma once
#include <Core/RTTIBaseClass.h>
#include <Input/InputFwd.h>

// Conditions that are evaluated by processing input events.
// Event conditions return true if triggered by event.

namespace Input
{

class CInputConditionEvent: public Core::CRTTIBaseClass
{
	RTTI_CLASS_DECL(Input::CInputConditionEvent, Core::CRTTIBaseClass);

public:

	virtual bool UpdateParams(std::function<std::string(const char*)> ParamGetter, std::set<std::string>* pOutParams) { OK; }
	virtual void Reset() { /* Most of events are stateless */ }
	virtual UPTR OnAxisMove(const IInputDevice* pDevice, const Event::AxisMove& Event) { return 0; }
	virtual UPTR OnButtonDown(const IInputDevice* pDevice, const Event::ButtonDown& Event) { return 0; }
	virtual UPTR OnButtonUp(const IInputDevice* pDevice, const Event::ButtonUp& Event) { return 0; }
	virtual UPTR OnTextInput(const IInputDevice* pDevice, const Event::TextInput& Event) { return 0; }
	virtual UPTR OnTimeElapsed(float ElapsedTime) { return 0; }
};

typedef std::unique_ptr<CInputConditionEvent> PInputConditionEvent;

}
