#include "InputMappingEvent.h"

#include <Input/InputServer.h>
#include <Input/Events/KeyDown.h>
#include <Input/Events/KeyUp.h>
#include <Input/Events/CharInput.h>
#include <Input/Events/MouseBtnDown.h>
#include <Input/Events/MouseBtnUp.h>
#include <Input/Events/MouseDoubleClick.h>
#include <Input/Events/MouseWheel.h>
#include <Events/EventServer.h>
#include <Data/Params.h>

namespace Input
{

bool CInputMappingEvent::Init(CStrID Name, const CParams& Desc)
{
	const CString& InEvent = Desc.Get<CString>(CStrID("InEvent"));
	if (InEvent == "KeyDown")
	{
		InEventID = &Event::KeyDown::RTTI;
		Key = (EKey)Desc.Get<int>(CStrID("Key"));
	}
	else if (InEvent == "KeyUp")
	{
		InEventID = &Event::KeyUp::RTTI;
		Key = (EKey)Desc.Get<int>(CStrID("Key"));
	}
	else if (InEvent == "Char")
	{
		InEventID = &Event::CharInput::RTTI;
		Char = (ushort)Desc.Get<int>(CStrID("Char"));
	}
	else if (InEvent == "MouseBtnDown")
	{
		InEventID = &Event::MouseBtnDown::RTTI;
		MouseBtn = (EMouseButton)Desc.Get<int>(CStrID("MouseBtn"));
	}
	else if (InEvent == "MouseBtnUp")
	{
		InEventID = &Event::MouseBtnUp::RTTI;
		MouseBtn = (EMouseButton)Desc.Get<int>(CStrID("MouseBtn"));
	}
	else if (InEvent == "MouseDoubleClick")
	{
		InEventID = &Event::MouseDoubleClick::RTTI;
		MouseBtn = (EMouseButton)Desc.Get<int>(CStrID("MouseBtn"));
	}
	else if (InEvent == "MouseWheelFwd")
	{
		InEventID = &Event::MouseWheel::RTTI;
		WheelFwd = true;
	}
	else if (InEvent == "MouseWheelBack")
	{
		InEventID = &Event::MouseWheel::RTTI;
		WheelFwd = false;
	}
	else n_error("CInputMappingEvent::Init -> Unsupported input event!");

	OutEventID = Name;

	OK;
}
//---------------------------------------------------------------------

bool CInputMappingEvent::Init(CStrID Name, int KeyCode)
{
	InEventID = &Event::KeyUp::RTTI;
	Key = (EKey)KeyCode;
	OutEventID = Name;
	OK;
}
//---------------------------------------------------------------------

void CInputMappingEvent::Enable()
{
	if (Sub_InputEvent.IsValid()) return;

	bool(CInputMappingEvent::*CB)(const CEventBase& Event);

	if (InEventID == &Event::KeyDown::RTTI) CB = &CInputMappingEvent::OnKeyDown;
	else if (InEventID == &Event::KeyUp::RTTI) CB = &CInputMappingEvent::OnKeyUp;
	else if (InEventID == &Event::CharInput::RTTI) CB = &CInputMappingEvent::OnCharInput;
	else if (InEventID == &Event::MouseBtnDown::RTTI) CB = &CInputMappingEvent::OnMouseBtnDown;
	else if (InEventID == &Event::MouseBtnUp::RTTI) CB = &CInputMappingEvent::OnMouseBtnUp;
	else if (InEventID == &Event::MouseDoubleClick::RTTI) CB = &CInputMappingEvent::OnMouseDoubleClick;
	else if (InEventID == &Event::MouseWheel::RTTI) CB = &CInputMappingEvent::OnMouseWheel;
	else n_error("Unsupported input event!");

	Sub_InputEvent = InputSrv->Subscribe(InEventID, this, CB, InputPriority_Mapping);
}
//---------------------------------------------------------------------

void CInputMappingEvent::Disable()
{
	Sub_InputEvent = NULL;
}
//---------------------------------------------------------------------

bool CInputMappingEvent::OnKeyDown(const Events::CEventBase& Event)
{
	if (Key == ((const Event::KeyDown&)Event).ScanCode) return !!EventSrv->FireEvent(OutEventID);
	FAIL;
}
//---------------------------------------------------------------------

bool CInputMappingEvent::OnKeyUp(const Events::CEventBase& Event)
{
	if (Key == ((const Event::KeyUp&)Event).ScanCode) return !!EventSrv->FireEvent(OutEventID);
	FAIL;
}
//---------------------------------------------------------------------

bool CInputMappingEvent::OnCharInput(const Events::CEventBase& Event)
{
	if (Char == ((const Event::CharInput&)Event).Char) return !!EventSrv->FireEvent(OutEventID);
	FAIL;
}
//---------------------------------------------------------------------

bool CInputMappingEvent::OnMouseBtnDown(const Events::CEventBase& Event)
{
	if (MouseBtn == ((const Event::MouseBtnDown&)Event).Button) return !!EventSrv->FireEvent(OutEventID);
	FAIL;
}
//---------------------------------------------------------------------

bool CInputMappingEvent::OnMouseBtnUp(const Events::CEventBase& Event)
{
	if (MouseBtn == ((const Event::MouseBtnUp&)Event).Button) return !!EventSrv->FireEvent(OutEventID);
	FAIL;
}
//---------------------------------------------------------------------

bool CInputMappingEvent::OnMouseDoubleClick(const Events::CEventBase& Event)
{
	if (MouseBtn == ((const Event::MouseDoubleClick&)Event).Button) return !!EventSrv->FireEvent(OutEventID);
	FAIL;
}
//---------------------------------------------------------------------

bool CInputMappingEvent::OnMouseWheel(const Events::CEventBase& Event)
{
	if (WheelFwd == (((const Event::MouseWheel&)Event).Delta > 0)) return !!EventSrv->FireEvent(OutEventID);
	FAIL;
}
//---------------------------------------------------------------------

} // namespace Input
