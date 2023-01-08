#pragma once
#include <Input/InputConditionEvent.h>

// Event condition that is triggered by receiving a text input

namespace Input
{

class CInputConditionText: public CInputConditionEvent
{
	RTTI_CLASS_DECL(Input::CInputConditionText, Input::CInputConditionEvent);

protected:

	std::string _Text;
	std::string _Accumulated;
	bool _AllowPartial = false;

public:

	CInputConditionText(std::string Text, bool AllowPartial = false);

	virtual UPTR OnTextInput(const IInputDevice* pDevice, const Event::TextInput& Event) override;
};

}
