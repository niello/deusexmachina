#pragma once
#include <Input/InputConditionState.h>
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

	virtual bool	Initialize(const Data::CParams& Desc) override;
	virtual void	Reset() override;
	virtual void	OnAxisMove(const IInputDevice* pDevice, const Event::AxisMove& Event) override;
	virtual void	OnButtonDown(const IInputDevice* pDevice, const Event::ButtonDown& Event) override;
	virtual void	OnButtonUp(const IInputDevice* pDevice, const Event::ButtonUp& Event) override;
	virtual void	OnTextInput(const IInputDevice* pDevice, const Event::TextInput& Event) override;
	virtual void	OnTimeElapsed(float ElapsedTime) override;
};

}
