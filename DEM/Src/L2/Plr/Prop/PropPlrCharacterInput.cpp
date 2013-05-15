#include "PropPlrCharacterInput.h"

#include <UI/Prop/PropUIControl.h>
#include <AI/Movement/Tasks/TaskGoto.h>
#include <AI/Events/QueueTask.h>

#include <Events/EventManager.h>
#include <Input/InputServer.h>
#include <Input/Events/MouseBtnDown.h>
#include <Input/Events/MouseBtnUp.h>
#include <Input/Events/MouseDoubleClick.h>
#include <Input/Events/MouseMoveRaw.h>
#include <Input/Events/MouseWheel.h>
#include <Game/Mgr/FocusManager.h>
#include <Game/Mgr/EnvQueryManager.h>
#include <Camera/Event/CameraOrbit.h>
#include <Camera/Event/CameraDistance.h>

#include <Loading/EntityFactory.h>

namespace Properties
{
__ImplementClassNoFactory(Properties::CPropPlrCharacterInput, Properties::CPropInput);
__ImplementClass(Properties::CPropPlrCharacterInput);
RegisterProperty(CPropPlrCharacterInput);

using namespace Data;
using namespace Input;

void CPropPlrCharacterInput::ActivateInput()
{
	const int InputPriority_FocusChar = InputPriority_Raw + 1;

	SUBSCRIBE_INPUT_EVENT(MouseBtnDown, CPropPlrCharacterInput, OnMouseBtnDown, InputPriority_FocusChar);
	SUBSCRIBE_INPUT_EVENT(MouseBtnUp, CPropPlrCharacterInput, OnMouseBtnUp, InputPriority_FocusChar);
	SUBSCRIBE_INPUT_EVENT(MouseDoubleClick, CPropPlrCharacterInput, OnMouseDoubleClick, InputPriority_FocusChar);
	SUBSCRIBE_INPUT_EVENT(MouseWheel, CPropPlrCharacterInput, OnMouseWheel, InputPriority_FocusChar);
}
//---------------------------------------------------------------------

void CPropPlrCharacterInput::DeactivateInput()
{
	UNSUBSCRIBE_EVENT(MouseBtnDown);
	UNSUBSCRIBE_EVENT(MouseBtnUp);
	UNSUBSCRIBE_EVENT(MouseDoubleClick);
	UNSUBSCRIBE_EVENT(MouseMoveRaw);
	UNSUBSCRIBE_EVENT(MouseWheel);
}
//---------------------------------------------------------------------

bool CPropPlrCharacterInput::OnMouseBtnDown(const Events::CEventBase& Event)
{
	const Event::MouseBtnDown& Ev = (const Event::MouseBtnDown&)Event;
	if (Ev.Button == Input::MBMiddle)
	{
		SUBSCRIBE_INPUT_EVENT(MouseMoveRaw, CPropPlrCharacterInput, OnMouseMoveRaw, InputPriority_Raw);
		FAIL; //???OK?
	}
	return OnMouseClick(Ev.Button);
}
//---------------------------------------------------------------------

bool CPropPlrCharacterInput::OnMouseBtnUp(const Events::CEventBase& Event)
{
	if (((const Event::MouseBtnUp&)Event).Button == Input::MBMiddle) UNSUBSCRIBE_EVENT(MouseMoveRaw);
	FAIL; //???OK?
}
//---------------------------------------------------------------------

bool CPropPlrCharacterInput::OnMouseDoubleClick(const Events::CEventBase& Event)
{
	return OnMouseClick(((const Event::MouseDoubleClick&)Event).Button, true);
}
//---------------------------------------------------------------------

bool CPropPlrCharacterInput::OnMouseMoveRaw(const Events::CEventBase& Event)
{
	//!!!TO SETTINGS!
	static const float PlrCameraSens = 0.05f;

	const Event::MouseMoveRaw& Ev = (const Event::MouseMoveRaw&)Event;
	GetEntity()->FireEvent(Event::CameraOrbit(Ev.X * PlrCameraSens, Ev.Y * PlrCameraSens));
	FAIL;
}
//---------------------------------------------------------------------

bool CPropPlrCharacterInput::OnMouseWheel(const Events::CEventBase& Event)
{
	const Event::MouseWheel& Ev = (const Event::MouseWheel&)Event;
	GetEntity()->FireEvent(Event::CameraDistance((float)(-Ev.Delta)));
	FAIL; //???!!!OK?!
}
//---------------------------------------------------------------------

bool CPropPlrCharacterInput::OnMouseClick(Input::EMouseButton Button, bool Double)
{
    if (EnvQueryMgr->HasMouseIntersection())
    {
		Game::CEntity* pEnt = EnvQueryMgr->GetEntityUnderMouse();
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
				Task->Point = EnvQueryMgr->GetMousePos3D();
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

} // namespace Properties
