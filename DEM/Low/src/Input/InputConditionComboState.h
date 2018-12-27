#pragma once
#ifndef __DEM_L1_INPUT_CONDITION_COMBO_STATE_H__
#define __DEM_L1_INPUT_CONDITION_COMBO_STATE_H__

#include <Input/InputCondition.h>
#include <Data/FixedArray.h>

// State condition that is On when all its child conditions are On

namespace Input
{

class CInputConditionComboState: public CInputConditionState
{
	__DeclareClass(CInputConditionComboState);

protected:

	CFixedArray<CInputConditionState*>	Children;

	void			Clear();

public:

	virtual ~CInputConditionComboState() { Clear(); }

	virtual bool	Initialize(const Data::CParams& Desc);
	virtual void	Reset();
	virtual void	OnAxisMove(const IInputDevice* pDevice, const Event::AxisMove& Event);
	virtual void	OnButtonDown(const IInputDevice* pDevice, const Event::ButtonDown& Event);
	virtual void	OnButtonUp(const IInputDevice* pDevice, const Event::ButtonUp& Event);
	virtual void	OnTimeElapsed(float ElapsedTime);
};

}

#endif
