#include "PropInput.h"

namespace Prop
{
__ImplementClass(Prop::CPropInput, 'PRIN', Game::CProperty);
__ImplementPropertyStorage(CPropInput);

void CPropInput::Activate()
{
	CProperty::Activate();

	PROP_SUBSCRIBE_PEVENT(EnableInput, CPropInput, OnEnableInput);
	PROP_SUBSCRIBE_PEVENT(DisableInput, CPropInput, OnDisableInput);

	if (Enabled /*&& HasFocus()*/) ActivateInput();
}
//---------------------------------------------------------------------

void CPropInput::Deactivate()
{
	UNSUBSCRIBE_EVENT(EnableInput);
	UNSUBSCRIBE_EVENT(DisableInput);

	CProperty::Deactivate();
}
//---------------------------------------------------------------------

bool CPropInput::OnEnableInput(const Events::CEventBase& Event)
{
	EnableInput(true);
	OK;
}
//---------------------------------------------------------------------

bool CPropInput::OnDisableInput(const Events::CEventBase& Event)
{
	EnableInput(false);
	OK;
}
//---------------------------------------------------------------------

bool CPropInput::HasFocus() const
{
	FAIL; //return FocusMgr->GetInputFocusEntity() == GetEntity();
}
//---------------------------------------------------------------------

void CPropInput::EnableInput(bool Enable)
{
	if (Enable != Enabled)
	{
		if (HasFocus())
		{
			if (Enable) ActivateInput();
			else DeactivateInput();
		}
		Enabled = Enable;
	}
}
//---------------------------------------------------------------------

} // namespace Prop
