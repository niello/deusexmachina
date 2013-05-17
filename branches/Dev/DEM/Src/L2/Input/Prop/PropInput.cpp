#include "PropInput.h"

#include <Game/Mgr/FocusManager.h>

namespace Prop
{
__ImplementClass(Prop::CPropInput, 'PRIN', Game::CProperty);
__ImplementPropertyStorage(CPropInput);

void CPropInput::Activate()
{
	CProperty::Activate();

	PROP_SUBSCRIBE_PEVENT(OnObtainInputFocus, CPropInput, OnObtainInputFocus);
	PROP_SUBSCRIBE_PEVENT(OnLoseInputFocus, CPropInput, OnLoseInputFocus);
	PROP_SUBSCRIBE_PEVENT(EnableInput, CPropInput, OnEnableInput);
	PROP_SUBSCRIBE_PEVENT(DisableInput, CPropInput, OnDisableInput);

	if (Enabled && HasFocus()) ActivateInput();
}
//---------------------------------------------------------------------

void CPropInput::Deactivate()
{
	if (HasFocus()) FocusMgr->SetInputFocusEntity(NULL);

	UNSUBSCRIBE_EVENT(OnObtainInputFocus);
	UNSUBSCRIBE_EVENT(OnLoseInputFocus);
	UNSUBSCRIBE_EVENT(EnableInput);
	UNSUBSCRIBE_EVENT(DisableInput);

	CProperty::Deactivate();
}
//---------------------------------------------------------------------

bool CPropInput::OnObtainInputFocus(const Events::CEventBase& Event)
{
	if (Enabled) ActivateInput();
	OK;
}
//---------------------------------------------------------------------

bool CPropInput::OnLoseInputFocus(const Events::CEventBase& Event)
{
	DeactivateInput();
	OK;
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
	return FocusMgr->GetInputFocusEntity() == GetEntity();
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
