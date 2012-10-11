#include "CSharpUIEventHandler.h"

#include <Events/EventManager.h>
#include <Input/InputServer.h>
#include <Input/Events/MouseBtnDown.h>
#include <Input/Events/MouseBtnUp.h>

namespace App
{

CCSharpUIEventHandler::CCSharpUIEventHandler():
	OnEntitySelectedCB(NULL),
	MouseButtonCB(NULL)
{
	SUBSCRIBE_PEVENT(OnEntitySelected, CCSharpUIEventHandler, OnEntitySelected);
	SUBSCRIBE_INPUT_EVENT(MouseBtnDown, CCSharpUIEventHandler, OnMouseBtnDown, Input::InputPriority_Raw);
	SUBSCRIBE_INPUT_EVENT(MouseBtnUp, CCSharpUIEventHandler, OnMouseBtnUp, Input::InputPriority_Raw);
}
//---------------------------------------------------------------------

bool CCSharpUIEventHandler::OnEntitySelected(const Events::CEventBase& Event)
{
	if (!OnEntitySelectedCB) FAIL;
	OnEntitySelectedCB(((const Events::CEvent&)Event).Params->Get<CStrID>(CStrID("UID"), CStrID::Empty).CStr());
	OK;
}
//---------------------------------------------------------------------

bool CCSharpUIEventHandler::OnMouseBtnDown(const Events::CEventBase& Event)
{
	if (!MouseButtonCB) FAIL;
	const Event::MouseBtnDown& Ev = ((const Event::MouseBtnDown&)Event);
	MouseButtonCB(Ev.X, Ev.Y, (int)Ev.Button, Down);
	OK;
}
//---------------------------------------------------------------------

bool CCSharpUIEventHandler::OnMouseBtnUp(const Events::CEventBase& Event)
{
	if (!MouseButtonCB) FAIL;
	const Event::MouseBtnUp& Ev = ((const Event::MouseBtnUp&)Event);
	MouseButtonCB(Ev.X, Ev.Y, (int)Ev.Button, Up);
	OK;
}
//---------------------------------------------------------------------

}