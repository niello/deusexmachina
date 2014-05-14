#pragma once
#ifndef __DEM_L1_INPUT_SERVER_H__
#define __DEM_L1_INPUT_SERVER_H__

#include <Events/EventDispatcher.h>
#include <Events/EventsFwd.h>
#include <Data/Singleton.h>
#include <Input/Keys.h>
#include <Input/ControlLayout.h>
#include <Data/Dictionary.h>

// The input server is the central object of the input subsystem

#define DECLARE_ALL_INPUT_HANDLER(HandlerName) \
	bool HandlerName(const Events::CEventBase& Event); \
	Events::PSub Sub_ALL_INPUT;

#define SUBSCRIBE_INPUT_EVENT(EventName, Class, Handler, Priority) \
	InputSrv->Subscribe<Class>(&Event::EventName::RTTI, this, &Class::Handler, &Sub_##EventName, Priority)

#define SUBSCRIBE_ALL_INPUT(Class, Handler, Priority) \
	InputSrv->Subscribe<Class>(NULL, this, &Class::Handler, &Sub_ALL_INPUT, Priority)

#define UNBSUBSCRIBE_ALL_INPUT \
	Sub_ALL_INPUT = NULL

#define KEY_IS_DOWN			0x01
#define KEY_IS_UP			0x02
#define KEY_IS_PRESSED		0x04
#define KEY_IS_DBL_CLICKED	0x08

namespace Input
{
using namespace Events;

const int CharBufferSize = 256;

enum EInputPriority
{
	InputPriority_UI		= 100,	// UI must have high priority
	InputPriority_Mapping	= 5,
	InputPriority_Raw		= 1
};

// Priorities:
// StateTracker - highest, always FAIL
// Capture - next, always OK on its events. If someone begins capturing, reset other listeners of captured events!
// GUI - next, CEGUI listeners, OK if input processed by CEGUI
// Game - player control, world clicking etc

// Device IDs:
//#define N_IDEV_NONE         (0)
//#define N_IDEV_KEYBOARD     (1<<16)     ///< the system keyboard
//#define N_IDEV_MOUSE        (1<<17)     ///< the system mouse
//
//#define N_INPUT_KEYBOARD(x)     (N_IDEV_KEYBOARD | x)
//#define N_INPUT_MOUSE(x)        (N_IDEV_MOUSE | x)

#define InputSrv Input::CInputServer::Instance()

class CInputServer: public CEventDispatcher //!!!CPriorityEventDispatcher!
{
	__DeclareClassNoFactory;
	__DeclareSingleton(CInputServer);

private:

//reset()
//set mouse sens, invert
//long pressed time / time of event in polling devices
//reactions on obtain/lose focus
//mapping to event/state

	bool	_IsOpen;

	uchar	KeyState[KeyCount];
	ushort	Chars[CharBufferSize];
	int		CharCount;

	uchar	MouseBtnState[MouseBtnCount];
	int		WheelFwd;
	int		WheelBack;
	int		RawMouseMoveX;
	int		RawMouseMoveY;
	int		MouseXAbs;
	int		MouseYAbs;
	float	MouseXRel;
	float	MouseYRel;

	CDict<CStrID, PControlLayout>	Layouts;
	CDict<CStrID, PControlLayout>	Contexts;

	//!!!save & load mappings for profile!

	CControlLayout*	LoadControlLayout(CStrID Name);

	DECLARE_EVENT_HANDLER(DisplayInput, OnDisplayInput);
	DECLARE_EVENT_HANDLER(OnDisplaySetFocus, OnDisplaySetFocus);
	DECLARE_EVENT_HANDLER(OnDisplayKillFocus, OnDisplayKillFocus);
	//!!!Handle display minimize & restore too (the same handling), but mb setfocus/killfocus are called in these cases?

public:

	CInputServer();
	~CInputServer();

	bool			Open();
	void			Close();
	void			Reset();
	void			Trigger();

	bool			IsOpen() const { return _IsOpen; }

	CControlLayout*	GetControlLayout(CStrID Name);
	bool			SetContextLayout(CStrID Context, CStrID Layout);
	CControlLayout*	GetContextLayout(CStrID Context) const;
	bool			EnableContext(CStrID Context, bool DisableOthers = false);
	void			DisableContext(CStrID Context);
	bool			IsContextEnabled(CStrID Context) const;

	bool			IsKeyDown(EKey Key) const { return !!(KeyState[Key] & KEY_IS_DOWN); }
	bool			IsKeyUp(EKey Key) const { return !!(KeyState[Key] & KEY_IS_UP); }
	bool			IsKeyPressed(EKey Key) const { return !!(KeyState[Key] & KEY_IS_PRESSED); }
	bool			CheckKeyState(EKey Key, uchar StateFlags) const { return !!(KeyState[Key] & StateFlags); }
	const ushort*	GetCharInput() { n_assert(CharCount < CharBufferSize); Chars[CharCount] = 0; return Chars; }

	bool			IsMouseBtnDown(EMouseButton Btn) const { return !!(MouseBtnState[Btn] & KEY_IS_DOWN); }
	bool			IsMouseBtnUp(EMouseButton Btn) const { return !!(MouseBtnState[Btn] & KEY_IS_UP); }
	bool			IsMouseBtnPressed(EMouseButton Btn) const { return !!(MouseBtnState[Btn] & KEY_IS_PRESSED); }
	bool			IsMouseBtnDoubleClicked(EMouseButton Btn) const { return !!(MouseBtnState[Btn] & KEY_IS_DBL_CLICKED); }
	bool			CheckMouseBtnState(EMouseButton Btn, uchar StateFlags) const { return !!(MouseBtnState[Btn] & StateFlags); }
	int				GetMousePosX() const { return MouseXAbs; }
	int				GetMousePosY() const { return MouseYAbs; }
	void			GetMousePos(int& X, int& Y) const { X = MouseXAbs; Y = MouseYAbs; }
	void			GetMousePosRel(float& X, float& Y) const { X = MouseXRel; Y = MouseYRel; }
	int				GetRawMouseMoveX() const { return RawMouseMoveX; }
	int				GetRawMouseMoveY() const { return RawMouseMoveY; }
	void			GetRawMouseMove(int& X, int& Y) const { X = RawMouseMoveX; Y = RawMouseMoveY; }
	int				GetWheelForward() const { return WheelFwd; }
	int				GetWheelBackward() const { return WheelBack; }
	int				GetWheelTotal() const { return WheelFwd + WheelBack; }
};

}

#endif
