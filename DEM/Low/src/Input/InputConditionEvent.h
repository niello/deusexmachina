#pragma once
#include <Core/RTTIBaseClass.h>
#include <Input/InputFwd.h>

// Conditions that are evaluated by processing input events.
// Event conditions return true if triggered by event.

namespace Input
{

class CInputConditionEvent: public Core::CRTTIBaseClass
{
	RTTI_CLASS_DECL;

public:

	virtual ~CInputConditionEvent() {}

	virtual bool	UpdateParams(std::function<std::string(const char*)> ParamGetter, std::set<std::string>* pOutParams) { OK; }
	virtual void	Reset() {}
	virtual bool	OnAxisMove(const IInputDevice* pDevice, const Event::AxisMove& Event) { FAIL; }
	virtual bool	OnButtonDown(const IInputDevice* pDevice, const Event::ButtonDown& Event) { FAIL; }
	virtual bool	OnButtonUp(const IInputDevice* pDevice, const Event::ButtonUp& Event) { FAIL; }
	virtual bool	OnTextInput(const IInputDevice* pDevice, const Event::TextInput& Event) { FAIL; }
	virtual bool	OnTimeElapsed(float ElapsedTime) { FAIL; }
};

typedef std::unique_ptr<CInputConditionEvent> PInputConditionEvent;

}
