#pragma once
#ifndef __DEM_L1_INPUT_CONDITION_COMBO_EVENT_H__
#define __DEM_L1_INPUT_CONDITION_COMBO_EVENT_H__

#include <Input/InputCondition.h>
#include <Data/FixedArray.h>

// Event condition that is triggered when the child state is on and the child event is triggered.
// Useful for combinations like "[Ctrl + Shift] + Click" or "[MMB] + Mouse move".

namespace Input
{

class CInputConditionComboEvent: public CInputConditionEvent
{
	__DeclareClass(CInputConditionComboEvent);

protected:

	CInputConditionEvent*	pEvent;
	CInputConditionState*	pState;

	void			Clear();

public:

	CInputConditionComboEvent(): pEvent(NULL), pState(NULL) {}
	virtual ~CInputConditionComboEvent() { Clear(); }

	virtual bool	Initialize(const Data::CParams& Desc);
	virtual void	Reset();
	virtual bool	OnAxisMove(const IInputDevice* pDevice, const Event::AxisMove& Event);
	virtual bool	OnButtonDown(const IInputDevice* pDevice, const Event::ButtonDown& Event);
	virtual bool	OnButtonUp(const IInputDevice* pDevice, const Event::ButtonUp& Event);
	virtual bool	OnTimeElapsed(float ElapsedTime);
};

}

#endif
