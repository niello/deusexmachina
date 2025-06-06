#pragma once
#include <Core/RTTIBaseClass.h>
#include <Input/InputFwd.h>

// Conditions that are evaluated by processing input events.
// State conditions are on or off, this state is changed by events.

namespace Input
{

class CInputConditionState: public DEM::Core::CRTTIBaseClass
{
	RTTI_CLASS_DECL(Input::CInputConditionState, DEM::Core::CRTTIBaseClass);

protected:

	bool On = false;

public:

	virtual bool	UpdateParams(std::function<std::string(const char*)> ParamGetter, std::set<std::string>* pOutParams) { OK; }
	virtual void	Reset() = 0;
	virtual void	OnAxisMove(const IInputDevice* pDevice, const Event::AxisMove& Event) {}
	virtual void	OnButtonDown(const IInputDevice* pDevice, const Event::ButtonDown& Event) {}
	virtual void	OnButtonUp(const IInputDevice* pDevice, const Event::ButtonUp& Event) {}
	virtual void	OnTextInput(const IInputDevice* pDevice, const Event::TextInput& Event) {}
	virtual void	OnTimeElapsed(float ElapsedTime) {}

	bool			IsOn() const { return On; }
};

typedef std::unique_ptr<CInputConditionState> PInputConditionState;

}
