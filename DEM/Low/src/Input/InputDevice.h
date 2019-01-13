#pragma once
#include <Input/InputFwd.h>
#include <Events/EventDispatcher.h>
#include <Data/RefCounted.h>

// An interface for various input devices such as keyboards, mouses and gamepads.
// Converts specific input to AxisMove, ButtonDown and ButtonUp native events.

namespace Input
{
typedef Ptr<class IInputDevice> PInputDevice;

class IInputDevice: public Data::CRefCounted, public Events::CEventDispatcher
{
public:

	virtual EDeviceType	GetType() const = 0;
	virtual bool		IsOperational() const = 0;
	virtual U8			GetAxisCount() const = 0;
	virtual U8			GetAxisCode(const char* pAlias) const = 0;
	virtual const char*	GetAxisAlias(U8 Code) const = 0;
	virtual void		SetAxisSensitivity(U8 Code, float Sensitivity) = 0;
	virtual float		GetAxisSensitivity(U8 Code) const = 0;
	virtual U8			GetButtonCount() const = 0;
	virtual U8			GetButtonCode(const char* pAlias) const = 0;
	virtual const char*	GetButtonAlias(U8 Code) const = 0;
};

}
