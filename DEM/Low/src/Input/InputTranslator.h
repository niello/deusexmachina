#pragma once
#include <Events/EventDispatcher.h>
#include <Data/StringID.h>
#include <map>

// Input translator receives events from input devices and translates them into
// general purpose events accompanied with a user ID to which a translator belongs.
// Mappings are separated into input contexts which can be enabled or disabled individually.

namespace DEM::Core
{
	class CApplication;
}

namespace Input
{
class IInputDevice;
class CControlLayout;

class CInputTranslator: public Events::CEventDispatcher
{
private:

	struct CInputContext
	{
		CStrID                          ID;
		std::unique_ptr<CControlLayout> Layout;
		bool                            Enabled = false;
	};

	CStrID                     _UserID;
	std::vector<CInputContext> _Contexts;

	std::map<IInputDevice*, std::vector<Events::PSub>, std::less<>> _DeviceSubs;

	void			Clear();

	bool			OnAxisMove(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event);
	bool			OnButtonDown(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event);
	bool			OnButtonUp(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event);
	bool			OnTextInput(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event);

public:

	CInputTranslator(CStrID UserID);
	virtual ~CInputTranslator() override = default;

	bool			LoadSettings(const Data::CParams& Desc);
	bool			UpdateParams(const DEM::Core::CApplication& App, std::set<std::string>* pOutParams = nullptr);
	void			SetUserID(CStrID UserID) { _UserID = UserID; }

	bool			CreateContext(CStrID ID, bool Bypass = false);
	void			DestroyContext(CStrID ID);
	bool			HasContext(CStrID ID) const;
	CControlLayout* GetContextLayout(CStrID ID);	// Intentionally editable
	void			EnableContext(CStrID ID);
	void			DisableContext(CStrID ID);
	void			EnableAllContexts();
	void			DisableAllContexts();
	bool			IsContextEnabled(CStrID ID) const;

	void			ConnectToDevice(IInputDevice* pDevice, U16 Priority = 100);
	void			DisconnectFromDevice(const IInputDevice* pDevice);
	UPTR			GetConnectedDevices(std::vector<IInputDevice*>& OutDevices) const;
	bool            IsConnectedToDevice(const IInputDevice* pDevice) const;
	void            TransferAllDevices(CInputTranslator* pNewOwner);

	void			UpdateTime(float ElapsedTime);
	bool			CheckState(CStrID StateID) const;
	void			Reset(/*device type*/);
};

typedef std::unique_ptr<CInputTranslator> PInputTranslator;

}
