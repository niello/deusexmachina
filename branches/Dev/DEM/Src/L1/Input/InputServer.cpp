#include "InputServer.h"

//#include <Render/RenderServer.h>
#include <System/Events/OSInput.h>
#include <Input/Events/KeyDown.h>
#include <Input/Events/KeyUp.h>
#include <Input/Events/CharInput.h>
#include <Input/Events/MouseMove.h>
#include <Input/Events/MouseMoveRaw.h>
#include <Input/Events/MouseBtnDown.h>
#include <Input/Events/MouseBtnUp.h>
#include <Input/Events/MouseDoubleClick.h>
#include <Input/Events/MouseWheel.h>
#include <Events/EventServer.h>
#include <Data/DataServer.h>
#include <Time/TimeServer.h>

namespace Input
{
__ImplementClassNoFactory(Input::CInputServer, Core::CObject);
__ImplementSingleton(Input::CInputServer);

CInputServer::CInputServer(): CEventDispatcher(16), _IsOpen(false)
{
	__ConstructSingleton;
}
//---------------------------------------------------------------------

CInputServer::~CInputServer()
{
    n_assert(!_IsOpen);
	__DestructSingleton;
}
//---------------------------------------------------------------------

bool CInputServer::Open()
{
	n_assert(!_IsOpen);

	memset(KeyState, 0, sizeof(KeyState));
	CharCount = 0;

	memset(MouseBtnState, 0, sizeof(MouseBtnState));
	WheelFwd =
	WheelBack =
	RawMouseMoveX =
	RawMouseMoveY =
	MouseXAbs =
	MouseYAbs = 0;
	MouseXRel =
	MouseYRel = 0.f;

	SUBSCRIBE_NEVENT(OSInput, CInputServer, OnOSWindowInput);
	SUBSCRIBE_PEVENT(OnSetFocus, CInputServer, OnOSWindowSetFocus);
	SUBSCRIBE_PEVENT(OnKillFocus, CInputServer, OnOSWindowKillFocus);

	_IsOpen = true;
	OK;
}
//---------------------------------------------------------------------

void CInputServer::Close()
{
	n_assert(_IsOpen);

	UNSUBSCRIBE_EVENT(OSInput);
	UNSUBSCRIBE_EVENT(OnSetFocus);
	UNSUBSCRIBE_EVENT(OnKillFocus);

	//UnsubscribeAll();
	Contexts.Clear();
	Layouts.Clear();

	_IsOpen = false;
}
//---------------------------------------------------------------------

void CInputServer::Trigger()
{
	n_assert(_IsOpen);

	for (int i = 0; i < KeyCount; ++i)
		if (KeyState[i] & KEY_IS_UP) KeyState[i] &= ~(KEY_IS_DOWN | KEY_IS_UP | KEY_IS_PRESSED);
		else KeyState[i] &= ~KEY_IS_DOWN;

	CharCount = 0;

	for (int i = 0; i < MouseBtnCount; ++i)
		if (MouseBtnState[i] & KEY_IS_UP) MouseBtnState[i] &= ~(KEY_IS_DOWN | KEY_IS_UP | KEY_IS_PRESSED | KEY_IS_DBL_CLICKED);
		else MouseBtnState[i] &= ~(KEY_IS_DOWN | KEY_IS_DBL_CLICKED);

	WheelFwd =
	WheelBack =
	RawMouseMoveX =
	RawMouseMoveY = 0;

	FireEvent(CStrID("OnInputUpdated"));
}
//---------------------------------------------------------------------

CControlLayout* CInputServer::GetControlLayout(CStrID Name)
{
	int Idx = Layouts.FindIndex(Name);
	if (Idx == INVALID_INDEX) return LoadControlLayout(Name);
	else return Layouts.ValueAt(Idx);
}
//---------------------------------------------------------------------

CControlLayout* CInputServer::LoadControlLayout(CStrID Name)
{
	Data::PParams Desc = DataSrv->LoadPRM("Input:Layouts.prm");
	if (!Desc.IsValid()) return NULL;
	Desc = Desc->Get<Data::PParams>(Name);
	if (!Desc.IsValid()) return NULL;
	PControlLayout New = n_new(CControlLayout);
	if (!New->Init(*Desc.GetUnsafe())) return NULL;
	Layouts.Add(Name, New);
	return New.GetUnsafe();
}
//---------------------------------------------------------------------

bool CInputServer::SetContextLayout(CStrID Context, CStrID Layout)
{
	PControlLayout pNewCtx = GetControlLayout(Layout);
	if (!pNewCtx) FAIL;

	bool WasEnabled;

	PControlLayout* ppCtx = Contexts.Get(Context);
	if (ppCtx)
	{
		WasEnabled = (*ppCtx)->IsEnabled();
		(*ppCtx)->Disable();
		(*ppCtx) = pNewCtx;
	}
	else
	{
		Contexts.Add(Context, pNewCtx);
		WasEnabled = false;
	}

	if (WasEnabled)	pNewCtx->Enable();

	OK;
}
//---------------------------------------------------------------------

CControlLayout* CInputServer::GetContextLayout(CStrID Context) const
{
	PControlLayout* ppCtx = Contexts.Get(Context);
	return (ppCtx) ? *ppCtx : NULL;
}
//---------------------------------------------------------------------

bool CInputServer::EnableContext(CStrID Context, bool DisableOthers)
{
	if (DisableOthers)
		for (int i = 0; i < Contexts.GetCount(); ++i)
			if (Contexts.KeyAt(i) != Context)
				Contexts.ValueAt(i)->Disable();

	PControlLayout* ppCtx = Contexts.Get(Context);
	if (ppCtx)
	{
		(*ppCtx)->Enable();
		OK;
	}
	FAIL;
}
//---------------------------------------------------------------------

void CInputServer::DisableContext(CStrID Context)
{
	PControlLayout* ppCtx = Contexts.Get(Context);
	if (ppCtx) (*ppCtx)->Disable();
}
//---------------------------------------------------------------------

bool CInputServer::IsContextEnabled(CStrID Context) const
{
	PControlLayout* ppCtx = Contexts.Get(Context);
	return (ppCtx) ? (*ppCtx)->IsEnabled() : false;
}
//---------------------------------------------------------------------

void CInputServer::Reset()
{
	for (int i = 0; i < KeyCount; ++i)
		KeyState[i] = (KeyState[i] & KEY_IS_PRESSED) ? KEY_IS_UP : 0;

	CharCount = 0;

	for (int i = 0; i < MouseBtnCount; ++i)
		MouseBtnState[i] = (MouseBtnState[i] & KEY_IS_PRESSED) ? KEY_IS_UP : 0;

	WheelFwd =
	WheelBack =
	RawMouseMoveX =
	RawMouseMoveY = 0;

	FireEvent(CStrID("ResetInput"));
}
//---------------------------------------------------------------------

// Generates input events based on display input type & input settings like Invert Y, Sensitivity, axis smoothing etc
bool CInputServer::OnOSWindowInput(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	const Event::OSInput& Ev = (const Event::OSInput&)Event;

	switch (Ev.Type)
	{
		case Event::OSInput::KeyDown:
		{
			n_assert(Ev.KeyCode < KeyCount);
			uchar& State = KeyState[Ev.KeyCode];
			if (!(State & KEY_IS_PRESSED)) State |= (KEY_IS_DOWN | KEY_IS_PRESSED);
			FireEvent(Event::KeyDown(Ev.KeyCode), EV_TERM_ON_HANDLED);
			break;
		}

		case Event::OSInput::KeyUp:
			n_assert(Ev.KeyCode < KeyCount);
			KeyState[Ev.KeyCode] |= KEY_IS_UP;
			FireEvent(Event::KeyUp(Ev.KeyCode), EV_TERM_ON_HANDLED);
			break;

		case Event::OSInput::CharInput:
			n_assert(CharCount < CharBufferSize); // Last will be set to 0
			Chars[CharCount++] = Ev.Char;
			FireEvent(Event::CharInput(Ev.Char), EV_TERM_ON_HANDLED);
			break;

		case Event::OSInput::MouseMove:
			MouseXAbs = Ev.MouseInfo.x;
			MouseYAbs = Ev.MouseInfo.y;
			//!!!
			//RenderSrv->GetDisplay().GetRelativeXY(MouseXAbs, MouseYAbs, MouseXRel, MouseYRel);
			FireEvent(Event::MouseMove(MouseXAbs, MouseYAbs, MouseXRel, MouseYRel), EV_TERM_ON_HANDLED);
			break;

		case Event::OSInput::MouseMoveRaw:
			RawMouseMoveX += Ev.MouseInfo.x;
			RawMouseMoveY += Ev.MouseInfo.y;
			FireEvent(Event::MouseMoveRaw(Ev.MouseInfo.x, Ev.MouseInfo.y), EV_TERM_ON_HANDLED);
			break;

		case Event::OSInput::MouseDown:
			n_assert(Ev.MouseInfo.Button < MouseBtnCount);
			MouseBtnState[Ev.MouseInfo.Button] |= (KEY_IS_DOWN | KEY_IS_PRESSED);
			MouseXAbs = Ev.MouseInfo.x;
			MouseYAbs = Ev.MouseInfo.y;
			//!!!
			//RenderSrv->GetDisplay().GetRelativeXY(MouseXAbs, MouseYAbs, MouseXRel, MouseYRel);
			FireEvent(Event::MouseBtnDown(Ev.MouseInfo.Button, MouseXAbs, MouseYAbs, MouseXRel, MouseYRel), EV_TERM_ON_HANDLED);
			break;

		case Event::OSInput::MouseUp:
			n_assert(Ev.MouseInfo.Button < MouseBtnCount);
			MouseBtnState[Ev.MouseInfo.Button] |= KEY_IS_UP;
			MouseXAbs = Ev.MouseInfo.x;
			MouseYAbs = Ev.MouseInfo.y;
			//!!!
			//RenderSrv->GetDisplay().GetRelativeXY(MouseXAbs, MouseYAbs, MouseXRel, MouseYRel);
			FireEvent(Event::MouseBtnUp(Ev.MouseInfo.Button, MouseXAbs, MouseYAbs, MouseXRel, MouseYRel), EV_TERM_ON_HANDLED);
			break;

		case Event::OSInput::MouseDblClick:
			n_assert(Ev.MouseInfo.Button < MouseBtnCount);
			MouseBtnState[Ev.MouseInfo.Button] |= KEY_IS_DBL_CLICKED;
			MouseXAbs = Ev.MouseInfo.x;
			MouseYAbs = Ev.MouseInfo.y;
			//!!!
			//RenderSrv->GetDisplay().GetRelativeXY(MouseXAbs, MouseYAbs, MouseXRel, MouseYRel);
			FireEvent(Event::MouseDoubleClick(Ev.MouseInfo.Button, MouseXAbs, MouseYAbs, MouseXRel, MouseYRel), EV_TERM_ON_HANDLED);
			break;

		case Event::OSInput::MouseWheel:
			if (Ev.WheelDelta > 0) WheelFwd += Ev.WheelDelta;
			else WheelBack += Ev.WheelDelta;
			FireEvent(Event::MouseWheel(Ev.WheelDelta), EV_TERM_ON_HANDLED);
			break;
	}

	OK;
}
//---------------------------------------------------------------------

bool CInputServer::OnOSWindowSetFocus(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
#ifndef _DEBUG
	Reset();
#endif

	// release captures

	// input focus obtained event

	//???special Capture(handler)? call Subscribe with autoset priority, store PSub
	//ReleaseCapture - if first handler is Priority_Capture, remove it, or just free capturing PSub

	// control layout as input handler? map inside
	// maps rule to event name (+state?)
	// rule is one or more events in some order or without order
	// if all events happen in some period of time, event is fired && state is set to true
	// if state tracking enabled, state is set to false on rule is broken
	// mb it is better to poll when check rules, not to catch events?

	// There are input event & input state
	// event: button was down/up
	// state: button is pressed down or not
	// need to deal with both
	// this srv is responsible for events, handlers are responsible for high-level events & states

	// rules can be like:
	//   ctrl is down & k is down
	//   mouse is moved (more than x?) & a is down

	OK;
}
//---------------------------------------------------------------------

bool CInputServer::OnOSWindowKillFocus(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
#ifndef _DEBUG
	Reset();
#endif

	// release captures

	// input focus lost event

	OK;
}
//---------------------------------------------------------------------

}
