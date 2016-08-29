#pragma once
#ifndef __DEM_L1_INPUT_CONDITION_H__
#define __DEM_L1_INPUT_CONDITION_H__

#include <Core/RTTIBaseClass.h>
#include <Input/InputFwd.h>

// Conditions that are evaluated by processing input events.
// Event conditions return true if triggered by event.
// State conditions are on or off, this state is changed by events.

namespace Data
{
	class CParams;
}

namespace Input
{

class CInputConditionEvent: public Core::CRTTIBaseClass
{
	__DeclareClassNoFactory;

public:

	static CInputConditionEvent* CreateByType(const char* pType);

	virtual bool	Initialize(const Data::CParams& Desc) = 0;
	virtual void	Reset() {}
	virtual bool	OnAxisMove(const IInputDevice* pDevice, const Event::AxisMove& Event) { FAIL; }
	virtual bool	OnButtonDown(const IInputDevice* pDevice, const Event::ButtonDown& Event) { FAIL; }
	virtual bool	OnButtonUp(const IInputDevice* pDevice, const Event::ButtonUp& Event) { FAIL; }
	virtual bool	OnTimeElapsed(float ElapsedTime) { FAIL; }
};

class CInputConditionState: public Core::CRTTIBaseClass
{
	__DeclareClassNoFactory;

protected:

	bool On;

public:

	static CInputConditionState* CreateByType(const char* pType);

	CInputConditionState(): On(false) {}

	virtual bool	Initialize(const Data::CParams& Desc) = 0;
	virtual void	Reset() {}
	virtual void	OnAxisMove(const IInputDevice* pDevice, const Event::AxisMove& Event) {}
	virtual void	OnButtonDown(const IInputDevice* pDevice, const Event::ButtonDown& Event) {}
	virtual void	OnButtonUp(const IInputDevice* pDevice, const Event::ButtonUp& Event) {}
	virtual void	OnTimeElapsed(float ElapsedTime) {}

	bool			IsOn() const { return On; }
};

}

#endif
