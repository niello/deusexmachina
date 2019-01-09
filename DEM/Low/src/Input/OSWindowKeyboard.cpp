#include "OSWindowKeyboard.h"

#include <Input/InputEvents.h>
#include <System/Events/OSInput.h>
#include <System/OSWindow.h>

namespace Input
{

void COSWindowKeyboard::Attach(DEM::Sys::COSWindow* pOSWindow, U16 Priority)
{
	if (pOSWindow)
	{
		DISP_SUBSCRIBE_NEVENT_PRIORITY(pOSWindow, OSInput, COSWindowKeyboard, OnOSWindowInput, Priority);
	}
	else
	{
		UNSUBSCRIBE_EVENT(OSInput);
	}
	pWindow = pOSWindow;
}
//---------------------------------------------------------------------

U8 COSWindowKeyboard::GetButtonCode(const char* pAlias) const
{
	//!!!IMPLEMENT!
	return InvalidCode;
}
//---------------------------------------------------------------------

const char* COSWindowKeyboard::GetButtonAlias(U8 Code) const
{
	//!!!IMPLEMENT!
	return NULL;
}
//---------------------------------------------------------------------

//!!!Current approach is temporary and not robust across different keyboards and platforms!
static inline U8 ConvertToKeyCode(U8 ScanCode, U8 VirtualKey, bool IsExtended)
{
	//!!!IMPLEMENT PROPERLY!
	U8 KeyCode = ScanCode;
	if (IsExtended) KeyCode |= 0x80;
	return KeyCode;
}
//---------------------------------------------------------------------

bool COSWindowKeyboard::OnOSWindowInput(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	n_assert_dbg(Event.IsA<Event::OSInput>());

	const Event::OSInput& OSInputEvent = (const Event::OSInput&)Event;

	switch (OSInputEvent.Type)
	{
		case Event::OSInput::KeyDown:
		{
			// We are interested only in actual key down events
			if (!(OSInputEvent.KeyboardInfo.Flags & Event::OSInput::Key_Repeated))
			{
				U8 KeyCode = ConvertToKeyCode(
					OSInputEvent.KeyboardInfo.ScanCode,
					OSInputEvent.KeyboardInfo.VirtualKey,
					(OSInputEvent.KeyboardInfo.Flags & Event::OSInput::Key_Extended) != 0);
				Event::ButtonDown Ev(KeyCode);
				FireEvent(Ev);
			}
			OK;
		}

		case Event::OSInput::KeyUp:
		{
			U8 KeyCode = ConvertToKeyCode(
				OSInputEvent.KeyboardInfo.ScanCode,
				OSInputEvent.KeyboardInfo.VirtualKey,
				(OSInputEvent.KeyboardInfo.Flags & Event::OSInput::Key_Extended) != 0);
			Event::ButtonUp Ev(KeyCode);
			FireEvent(Ev);
			OK;
		}
	}

	FAIL;
}
//---------------------------------------------------------------------

}