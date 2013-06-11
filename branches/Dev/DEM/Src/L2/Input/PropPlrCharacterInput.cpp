#include "PropPlrCharacterInput.h"

#include <UI/PropUIControl.h>
#include <AI/Movement/Tasks/TaskGoto.h>
#include <AI/Events/QueueTask.h>
#include <Game/GameServer.h>
#include <Input/InputServer.h>
#include <Input/Events/MouseBtnDown.h>
#include <Input/Events/MouseDoubleClick.h>

namespace Prop
{
__ImplementClass(Prop::CPropPlrCharacterInput, 'PPIN', Game::CProperty);
__ImplementPropertyStorage(CPropPlrCharacterInput);

bool CPropPlrCharacterInput::InternalActivate()
{
	//PROP_SUBSCRIBE_PEVENT(EnableInput, CPropPlrCharacterInput, OnEnableInput);
	//PROP_SUBSCRIBE_PEVENT(DisableInput, CPropPlrCharacterInput, OnDisableInput);

	if (Enabled /*&& HasFocus()*/) ActivateInput();
	OK;
}
//---------------------------------------------------------------------

void CPropPlrCharacterInput::InternalDeactivate()
{
	//UNSUBSCRIBE_EVENT(EnableInput);
	//UNSUBSCRIBE_EVENT(DisableInput);

}
//---------------------------------------------------------------------

void CPropPlrCharacterInput::ActivateInput()
{
	const int InputPriority_FocusChar = Input::InputPriority_Raw + 1;

	SUBSCRIBE_INPUT_EVENT(MouseBtnDown, CPropPlrCharacterInput, OnMouseBtnDown, InputPriority_FocusChar);
	SUBSCRIBE_INPUT_EVENT(MouseDoubleClick, CPropPlrCharacterInput, OnMouseDoubleClick, InputPriority_FocusChar);
}
//---------------------------------------------------------------------

void CPropPlrCharacterInput::DeactivateInput()
{
	UNSUBSCRIBE_EVENT(MouseBtnDown);
	UNSUBSCRIBE_EVENT(MouseDoubleClick);
}
//---------------------------------------------------------------------

bool CPropPlrCharacterInput::OnMouseBtnDown(const Events::CEventBase& Event)
{
	const Event::MouseBtnDown& Ev = (const Event::MouseBtnDown&)Event;
	return OnMouseClick(Ev.Button);
}
//---------------------------------------------------------------------

bool CPropPlrCharacterInput::OnMouseDoubleClick(const Events::CEventBase& Event)
{
	return OnMouseClick(((const Event::MouseDoubleClick&)Event).Button, true);
}
//---------------------------------------------------------------------

bool CPropPlrCharacterInput::OnMouseClick(Input::EMouseButton Button, bool Double)
{
	if (GameSrv->HasMouseIntersection())
    {
		Game::CEntity* pEnt = GameSrv->GetEntityUnderMouse();
		CPropUIControl* pCtl = (pEnt) ? pEnt->GetProperty<CPropUIControl>() : NULL;

		if (pCtl)
		{
			if (Button == Input::MBLeft)
			{
				pCtl->ExecuteDefaultAction(GetEntity());
				OK;
			}
			else if (Button == Input::MBRight)
			{
				pCtl->ShowPopup(GetEntity());
				OK;
			}
		}
		else
		{
			if (Button == Input::MBLeft)
			{
				// Handle movement
				AI::PTaskGoto Task = n_new(AI::CTaskGoto);
				Task->Point = GameSrv->GetMousePos3D();
				Task->MinDistance = 0.f;
				Task->MaxDistance = 0.f;
				Task->MvmtType = Double ? AI::AIMvmt_Type_Walk : AI::AIMvmt_Type_Run;
				GetEntity()->FireEvent(Event::QueueTask(Task));
				OK;
			}
			else
			{
				//!!!can show non-ctl popup here!
			}
		}
    }

	FAIL;
}
//---------------------------------------------------------------------

} // namespace Prop
