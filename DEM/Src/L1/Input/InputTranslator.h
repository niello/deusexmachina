#pragma once
#ifndef __DEM_L1_INPUT_TRANSLATOR_H__
#define __DEM_L1_INPUT_TRANSLATOR_H__

#include <Events/Subscription.h>
#include <Data/StringID.h>
#include <Data/Array.h>

// Input translator receives events from input devices and translates it into
// general-purpose events accompanied with a player context translator is linked to.
// Mappings are separated into input contexts which can be enabled or disabled individually.

namespace Input
{
class IInputDevice;

class CInputTranslator
{
private:

	U8						PlayerUID;

	CArray<Events::PSub>	Subscriptions;

	//context: ID, enabled, layout
	//layout: array of virtual mappings (event, state, range and any other), can be loaded and saved
	//mapping: processes an event, returns if it was consumed, fires event (or fills state?)

	//???or one sub and distinguish by RTTI inside?
	bool	OnAxisMove(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event);
	bool	OnButtonDown(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event);
	bool	OnButtonUp(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event);

public:

	CInputTranslator(U8 PlayerID): PlayerUID(PlayerID) {}

	//SetContextLayout(CStrID ID, control layout source);
	void	EnableContext(CStrID ID);
	void	DisableContext(CStrID ID);
	void	EnableAllContexts();
	void	DisableAllContexts();
	bool	IsContextEnabled(CStrID ID) const;

	void	ConnectToDevice(IInputDevice* pDevice, U16 Priority = 100);
	void	DisconnectFromDevice(const IInputDevice* pDevice);

	//FireQueuedEvents(/*max count*/)
	//CheckState()
	//Reset(/*device type*/)
};

}

#endif
