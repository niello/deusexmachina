#pragma once
#ifndef __DEM_L1_INPUT_TRANSLATOR_H__
#define __DEM_L1_INPUT_TRANSLATOR_H__

#include <Events/EventDispatcher.h>
#include <Data/StringID.h>
#include <Data/Array.h>

// Input translator receives events from input devices and translates it into
// general-purpose events accompanied with a player context translator is linked to.
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

	U8						PlayerUID;
	CArray<CInputContext>	Contexts;
	CArray<Events::PSub>	DeviceSubs;

	//event queue: ID, possible axis amount value

	void			Clear();

	bool			OnAxisMove(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event);
	bool			OnButtonDown(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event);
	bool			OnButtonUp(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event);

public:

	CInputTranslator(U8 PlayerID): PlayerUID(PlayerID) { Contexts.SetKeepOrder(true); }
	~CInputTranslator() { Clear(); }

	bool			LoadSettings(const Data::CParams& Desc);
	bool			SaveSettings(Data::CParams& OutDesc) const;

	bool			CreateContext(CStrID ID);
	void			DestroyContext(CStrID ID);
	CControlLayout*	GetContextLayout(CStrID ID);	// Intentionally editable
	void			EnableContext(CStrID ID);
	void			DisableContext(CStrID ID);
	void			EnableAllContexts();
	void			DisableAllContexts();
	bool			IsContextEnabled(CStrID ID) const;

	void			ConnectToDevice(IInputDevice* pDevice, U16 Priority = 100);
	void			DisconnectFromDevice(const IInputDevice* pDevice);

	//FireQueuedEvents(/*max count*/)
	//CheckState(CStrID State) // search in all active contexts, false of not found
	//Reset(/*device type*/)
};

}

#endif
