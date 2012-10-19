#include "CSharpUIConnector.h"

#include <Events/EventManager.h>
#include <Input/InputServer.h>
#include <Input/Events/MouseBtnDown.h>
#include <Input/Events/MouseBtnUp.h>

namespace App
{

CCSharpUIConnector::CCSharpUIConnector():
	OnEntitySelectedCB(NULL),
	MouseButtonCB(NULL),
	StringInputCB(NULL)
{
	SUBSCRIBE_PEVENT(OnEntitySelected, CCSharpUIConnector, OnEntitySelected);
	SUBSCRIBE_INPUT_EVENT(MouseBtnDown, CCSharpUIConnector, OnMouseBtnDown, Input::InputPriority_Raw);
	SUBSCRIBE_INPUT_EVENT(MouseBtnUp, CCSharpUIConnector, OnMouseBtnUp, Input::InputPriority_Raw);
}
//---------------------------------------------------------------------

bool CCSharpUIConnector::OnEntitySelected(const Events::CEventBase& Event)
{
	if (!OnEntitySelectedCB) FAIL;
	OnEntitySelectedCB(((const Events::CEvent&)Event).Params->Get<CStrID>(CStrID("UID"), CStrID::Empty).CStr());
	OK;
}
//---------------------------------------------------------------------

bool CCSharpUIConnector::OnMouseBtnDown(const Events::CEventBase& Event)
{
	if (!MouseButtonCB) FAIL;
	const Event::MouseBtnDown& Ev = ((const Event::MouseBtnDown&)Event);
	MouseButtonCB(Ev.X, Ev.Y, (int)Ev.Button, Down);
	OK;
}
//---------------------------------------------------------------------

bool CCSharpUIConnector::OnMouseBtnUp(const Events::CEventBase& Event)
{
	if (!MouseButtonCB) FAIL;
	const Event::MouseBtnUp& Ev = ((const Event::MouseBtnUp&)Event);
	MouseButtonCB(Ev.X, Ev.Y, (int)Ev.Button, Up);
	OK;
}
//---------------------------------------------------------------------

}