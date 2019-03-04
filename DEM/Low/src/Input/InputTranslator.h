#pragma once
#include <Events/EventDispatcher.h>
#include <Data/StringID.h>
#include <Data/Array.h>

// Input translator receives events from input devices and translates it into
// general-purpose events accompanied with a user ID to which a translator belongs.
// Mappings are separated into input contexts which can be enabled or disabled individually.

namespace Input
{
class IInputDevice;
class CControlLayout;

class CInputTranslator: public Events::CEventDispatcher
{
private:

	struct CInputContext
	{
		CStrID			ID;
		CControlLayout*	pLayout;
		bool			Enabled;
	};

	CStrID					_UserID;
	CArray<CInputContext>	Contexts;
	CArray<Events::PSub>	DeviceSubs;
	CArray<Events::CEvent>	EventQueue;

	void			Clear();

	bool			OnAxisMove(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event);
	bool			OnButtonDown(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event);
	bool			OnButtonUp(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event);
	bool			OnTextInput(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event);

public:

	CInputTranslator(CStrID UserID);
	virtual ~CInputTranslator();

	bool			LoadSettings(const Data::CParams& Desc);
	bool			SaveSettings(Data::CParams& OutDesc) const;

	bool			CreateContext(CStrID ID, bool Bypass = false);
	void			DestroyContext(CStrID ID);
	bool			HasContext(CStrID ID) const;
	CControlLayout*	GetContextLayout(CStrID ID);	// Intentionally editable
	void			EnableContext(CStrID ID);
	void			DisableContext(CStrID ID);
	void			EnableAllContexts();
	void			DisableAllContexts();
	bool			IsContextEnabled(CStrID ID) const;

	void			ConnectToDevice(IInputDevice* pDevice, U16 Priority = 100);
	void			DisconnectFromDevice(const IInputDevice* pDevice);

	void			UpdateTime(float ElapsedTime);
	void			FireQueuedEvents(/*max count*/);
	bool			CheckState(CStrID StateID) const;
	void			Reset(/*device type*/);
};

}
