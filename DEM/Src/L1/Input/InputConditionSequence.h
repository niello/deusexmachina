#pragma once
#ifndef __DEM_L1_INPUT_CONDITION_SEQUENCE_H__
#define __DEM_L1_INPUT_CONDITION_SEQUENCE_H__

#include <Input/InputCondition.h>
#include <Data/FixedArray.h>

// Event condition that is triggered when its child events are triggered in order.
// Condition is reset if sequence is broken by receiving any other input.

namespace Input
{

class CInputConditionSequence: public CInputConditionEvent
{
	__DeclareClass(CInputConditionSequence);

protected:

	CFixedArray<CInputConditionEvent*>	Children;
	UPTR								CurrChild;

	void			Clear();

public:

	virtual ~CInputConditionSequence() { Clear(); }

	virtual bool	Initialize(const Data::CParams& Desc);
	virtual void	Reset();
	virtual bool	OnAxisMove(const IInputDevice* pDevice, const Event::AxisMove& Event);
	virtual bool	OnButtonDown(const IInputDevice* pDevice, const Event::ButtonDown& Event);
	virtual bool	OnButtonUp(const IInputDevice* pDevice, const Event::ButtonUp& Event);
	virtual bool	OnTimeElapsed(float ElapsedTime);
};

}

#endif
