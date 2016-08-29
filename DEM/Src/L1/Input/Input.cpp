#include "InputFwd.h"

namespace Input
{

static const char* pDeviceTypeString[] =
{
	"Keyboard",
	"Mouse",
	"Gamepad"
};

static const char* pMouseAxisString[] =
{
	"X",
	"Y",
	"Wheel1",
	"Wheel2"
};

static const char* pMouseButtonString[] =
{
	"LMB",
	"RMB",
	"MMB",
	"X1",
	"X2",
	"User"
};

const char*	DeviceTypeToString(EDeviceType Type)
{
	if (Type >= Dev_Count) return NULL;
	return pDeviceTypeString[Type];
}
//---------------------------------------------------------------------

EDeviceType	StringToDeviceType(const char* pName)
{
	for (UPTR i = 0; i < Dev_Count; ++i)
		if (n_stricmp(pName, pDeviceTypeString[i]))
			return (EDeviceType)i;
	return Dev_Invalid;
}
//---------------------------------------------------------------------

const char*	MouseAxisToString(EMouseAxis Axis)
{
	if (Axis >= MA_Count) return NULL;
	return pMouseAxisString[Axis];
}
//---------------------------------------------------------------------

EMouseAxis StringToMouseAxis(const char* pName)
{
	for (UPTR i = 0; i < MA_Count; ++i)
		if (n_stricmp(pName, pMouseAxisString[i]))
			return (EMouseAxis)i;
	return MA_Invalid;
}
//---------------------------------------------------------------------

const char*	MouseButtonToString(EMouseButton Button)
{
	if (Button > MB_User)
	{
		//UPTR UserIndex = Button - MB_User;
		//if (UserIndex > 99) return NULL;
		//char Buffer[]
		//will return ptr to local, implement through external buffer if needed at all
		return NULL;
	}
	return pMouseButtonString[Button];
}
//---------------------------------------------------------------------

EMouseButton StringToMouseButton(const char* pName)
{
	for (UPTR i = 0; i <= MB_User; ++i)
		if (n_stricmp(pName, pMouseButtonString[i]))
			return (EMouseButton)i;
	return MB_Invalid;
}
//---------------------------------------------------------------------

}