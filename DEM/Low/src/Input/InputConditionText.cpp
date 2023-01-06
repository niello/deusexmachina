#include "InputConditionText.h"
#include <Input/InputEvents.h>

namespace Input
{

CInputConditionText::CInputConditionText(std::string Text, bool AllowPartial)
	: _Text(Text)
	, _AllowPartial(AllowPartial)
{
}
//---------------------------------------------------------------------

UPTR CInputConditionText::OnTextInput(const IInputDevice* pDevice, const Event::TextInput& Event)
{
	if (_AllowPartial)
	{
		//???instead of accumulation, start with the full string and remove head parts until empty?
		// FIXME: implement, now proceed to non-partial logic
		NOT_IMPLEMENTED;
	}

	const bool Match = Event.CaseSensitive ?
		(Event.Text == _Text) :
		n_stricmp(Event.Text.c_str(), _Text.c_str());

	return Match ? 1 : 0;
}
//---------------------------------------------------------------------

}
