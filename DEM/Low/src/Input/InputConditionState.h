#pragma once
#include <Core/RTTIBaseClass.h>
#include <Input/InputFwd.h>

// Conditions that are evaluated by processing input events.
// State conditions are on or off, this state is changed by events.

namespace Data
{
	class CParams;
}

namespace Input
{

class CInputConditionState: public Core::CRTTIBaseClass
{
	__DeclareClassNoFactory;

protected:

	bool On = false;

public:

	static CInputConditionState* CreateByType(const char* pType);

	CInputConditionState() {}
	virtual ~CInputConditionState() {}

	virtual bool	Initialize(const Data::CParams& Desc) = 0;
	virtual void	Reset() = 0;
	virtual void	OnAxisMove(const IInputDevice* pDevice, const Event::AxisMove& Event) {}
	virtual void	OnButtonDown(const IInputDevice* pDevice, const Event::ButtonDown& Event) {}
	virtual void	OnButtonUp(const IInputDevice* pDevice, const Event::ButtonUp& Event) {}
	virtual void	OnTextInput(const IInputDevice* pDevice, const Event::TextInput& Event) {}
	virtual void	OnTimeElapsed(float ElapsedTime) {}

	bool			IsOn() const { return On; }
};

}
