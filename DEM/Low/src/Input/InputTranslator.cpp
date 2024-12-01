#include "InputTranslator.h"
#include <Input/InputEvents.h>
#include <Input/InputDevice.h>
#include <Input/ControlLayout.h>
#include <Core/Application.h>

namespace Input
{

CInputTranslator::CInputTranslator(CStrID UserID)
	: _UserID(UserID)
{
}
//---------------------------------------------------------------------

void CInputTranslator::Clear()
{
	_Contexts.clear();
	_DeviceSubs.clear();
}
//---------------------------------------------------------------------

// Doesn't remove contexts but overrides existing ones
bool CInputTranslator::LoadSettings(const Data::CParams& Desc)
{
	for (UPTR i = 0; i < Desc.GetCount(); ++i)
	{
		const Data::CParam& Prm = Desc.Get(i);

		CStrID ContextID = Prm.GetName();
		CreateContext(ContextID);

		const auto& ContextLayoutDesc = *Prm.GetValue<Data::PParams>().Get();
		CControlLayout* pLayout = GetContextLayout(ContextID);
		if (!pLayout || !pLayout->Initialize(ContextLayoutDesc)) FAIL;
	}

	OK;
}
//---------------------------------------------------------------------

bool CInputTranslator::UpdateParams(const DEM::Core::CApplication& App, std::set<std::string>* pOutParams)
{
	auto ParamGetter = [&App, UserID = _UserID](const char* pKey) -> std::string
	{
		CString Value = App.GetStringSetting(pKey, CString::Empty, UserID);
		return Value.IsValid() ? Value.CStr() : "";
	};

	bool Result = true;

	for (const auto& Context : _Contexts)
	{
		if (auto pLayout = Context.Layout.get())
		{
			for (auto& Pair : pLayout->States)
				Result &= Pair.second->UpdateParams(ParamGetter, pOutParams);

			for (auto& Rec : pLayout->Events)
				Result &= Rec.Event->UpdateParams(ParamGetter, pOutParams);
		}
	}

	return Result;
}
//---------------------------------------------------------------------

bool CInputTranslator::CreateContext(CStrID ID, bool Bypass)
{
	for (const auto& Context : _Contexts)
		if (Context.ID == ID) FAIL;

	CInputContext NewCtx;
	NewCtx.ID = ID;
	NewCtx.Enabled = false;
	NewCtx.Layout = Bypass ? nullptr : std::make_unique<CControlLayout>();
	_Contexts.push_back(std::move(NewCtx));
	OK;
}
//---------------------------------------------------------------------

void CInputTranslator::DestroyContext(CStrID ID)
{
	for (auto It = _Contexts.begin(); It != _Contexts.end(); ++It)
	{
		if ((*It).ID == ID)
		{
			_Contexts.erase(It);
			break;
		}
	}
}
//---------------------------------------------------------------------

bool CInputTranslator::HasContext(CStrID ID) const
{
	for (const auto& Context : _Contexts)
		if (Context.ID == ID)
			return true;

	return false;
}
//---------------------------------------------------------------------

CControlLayout* CInputTranslator::GetContextLayout(CStrID ID)
{
	for (const auto& Context : _Contexts)
		if (Context.ID == ID)
			return Context.Layout.get();
	return nullptr;
}
//---------------------------------------------------------------------

void CInputTranslator::EnableContext(CStrID ID)
{
	for (auto& Context : _Contexts)
	{
		if (Context.ID == ID)
		{
			Context.Enabled = true;
			break;
		}
	}
}
//---------------------------------------------------------------------

void CInputTranslator::DisableContext(CStrID ID)
{
	for (auto& Context : _Contexts)
	{
		if (Context.ID == ID)
		{
			Context.Enabled = false;
			if (Context.Layout)
				Context.Layout->Reset();
			break;
		}
	}
}
//---------------------------------------------------------------------

void CInputTranslator::ConnectToDevice(IInputDevice* pDevice, U16 Priority)
{
	if (!pDevice || IsConnectedToDevice(pDevice)) return;

	auto& DeviceSubs = _DeviceSubs[pDevice];

	if (pDevice->GetAxisCount() > 0)
		DeviceSubs.push_back(pDevice->Subscribe(&Event::AxisMove::RTTI, this, &CInputTranslator::OnAxisMove));

	if (pDevice->GetButtonCount() > 0)
	{
		DeviceSubs.push_back(pDevice->Subscribe(&Event::ButtonDown::RTTI, this, &CInputTranslator::OnButtonDown));
		DeviceSubs.push_back(pDevice->Subscribe(&Event::ButtonUp::RTTI, this, &CInputTranslator::OnButtonUp));
	}

	if (pDevice->CanInputText())
		DeviceSubs.push_back(pDevice->Subscribe(&Event::TextInput::RTTI, this, &CInputTranslator::OnTextInput));
}
//---------------------------------------------------------------------

void CInputTranslator::DisconnectFromDevice(const IInputDevice* pDevice)
{
	auto It = _DeviceSubs.find(pDevice);
	if (It != _DeviceSubs.cend()) _DeviceSubs.erase(It);
}
//---------------------------------------------------------------------

UPTR CInputTranslator::GetConnectedDevices(std::vector<IInputDevice*>& OutDevices) const
{
	const UPTR PrevCount = OutDevices.size();
	for (const auto& [pDevice, Subs] : _DeviceSubs)
		OutDevices.push_back(pDevice);
	return OutDevices.size() - PrevCount;
}
//---------------------------------------------------------------------

bool CInputTranslator::IsConnectedToDevice(const IInputDevice* pDevice) const
{
	return _DeviceSubs.find(pDevice) != _DeviceSubs.cend();
}
//---------------------------------------------------------------------

void CInputTranslator::TransferAllDevices(CInputTranslator* pNewOwner)
{
	if (pNewOwner)
		for (const auto& [pDevice, Subs] : _DeviceSubs)
			pNewOwner->ConnectToDevice(static_cast<IInputDevice*>(pDevice));

	_DeviceSubs.clear();
}
//---------------------------------------------------------------------

bool CInputTranslator::OnAxisMove(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	const Event::AxisMove& Ev = static_cast<const Event::AxisMove&>(Event);

	UPTR Handled = 0;

	for (const auto& Ctx : _Contexts)
	{
		if (!Ctx.Enabled) continue;

		if (auto pLayout = Ctx.Layout.get())
		{
			for (auto& Pair : pLayout->States)
				Pair.second->OnAxisMove(Ev.Device, Ev);

			for (auto& Rec : pLayout->Events)
			{
				const auto Count = Rec.Event->OnAxisMove(Ev.Device, Ev);
				if (Count && !Handled)
				{
					Events::CEvent NewEvent;
					NewEvent.ID = Rec.OutEventID;
					NewEvent.Params = n_new(Data::CParams(1));
					NewEvent.Params->Set<float>(CStrID("Amount"), Ev.Amount);
					for (UPTR i = 0; i < Count; ++i)
						Handled += FireEvent(NewEvent, Events::Event_TermOnHandled);
				}
			}
		}
		else if (!Handled)
		{
			// Bypass context
			Event::AxisMove BypassEvent(Ev.Device, Ev.Code, Ev.Amount, _UserID);
			Handled += FireEvent(BypassEvent, Events::Event_TermOnHandled);
		}
	}

	return Handled > 0;
}
//---------------------------------------------------------------------

bool CInputTranslator::OnButtonDown(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	const Event::ButtonDown& Ev = static_cast<const Event::ButtonDown&>(Event);

	UPTR Handled = 0;

	for (const auto& Ctx : _Contexts)
	{
		if (!Ctx.Enabled) continue;

		if (auto pLayout = Ctx.Layout.get())
		{
			for (auto& Pair : pLayout->States)
				Pair.second->OnButtonDown(Ev.Device, Ev);

			for (auto& Rec : pLayout->Events)
			{
				const auto Count = Rec.Event->OnButtonDown(Ev.Device, Ev);
				if (Count && !Handled)
				{
					Events::CEvent NewEvent;
					NewEvent.ID = Rec.OutEventID;
					for (UPTR i = 0; i < Count; ++i)
						Handled += FireEvent(NewEvent, Events::Event_TermOnHandled);
				}
			}
		}
		else if (!Handled)
		{
			// Bypass context
			Event::ButtonDown BypassEvent(Ev.Device, Ev.Code, _UserID);
			Handled += FireEvent(BypassEvent, Events::Event_TermOnHandled);
		}
	}

	return Handled > 0;
}
//---------------------------------------------------------------------

bool CInputTranslator::OnButtonUp(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	const Event::ButtonUp& Ev = static_cast<const Event::ButtonUp&>(Event);

	UPTR Handled = 0;

	for (const auto& Ctx : _Contexts)
	{
		if (!Ctx.Enabled) continue;

		if (auto pLayout = Ctx.Layout.get())
		{
			for (auto& Pair : pLayout->States)
				Pair.second->OnButtonUp(Ev.Device, Ev);

			for (auto& Rec : pLayout->Events)
			{
				const auto Count = Rec.Event->OnButtonUp(Ev.Device, Ev);
				if (Count && !Handled)
				{
					Events::CEvent NewEvent;
					NewEvent.ID = Rec.OutEventID;
					for (UPTR i = 0; i < Count; ++i)
						Handled += FireEvent(NewEvent, Events::Event_TermOnHandled);
				}
			}
		}
		else if (!Handled)
		{
			// Bypass context
			Event::ButtonUp BypassEvent(Ev.Device, Ev.Code, _UserID);
			Handled += FireEvent(BypassEvent, Events::Event_TermOnHandled);
		}
	}

	return Handled > 0;
}
//---------------------------------------------------------------------

bool CInputTranslator::OnTextInput(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	const Event::TextInput& Ev = static_cast<const Event::TextInput&>(Event);

	UPTR Handled = 0;

	for (const auto& Ctx : _Contexts)
	{
		if (!Ctx.Enabled) continue;

		if (auto pLayout = Ctx.Layout.get())
		{
			for (auto& Pair : pLayout->States)
				Pair.second->OnTextInput(Ev.Device, Ev);

			for (auto& Rec : pLayout->Events)
			{
				const auto Count = Rec.Event->OnTextInput(Ev.Device, Ev);
				if (Count && !Handled)
				{
					Events::CEvent NewEvent;
					NewEvent.ID = Rec.OutEventID;
					for (UPTR i = 0; i < Count; ++i)
						Handled += FireEvent(NewEvent, Events::Event_TermOnHandled);
				}
			}
		}
		else if (!Handled)
		{
			// Bypass context
			Event::TextInput BypassEvent(Ev.Device, Ev.Text, Ev.CaseSensitive, _UserID);
			Handled += FireEvent(BypassEvent, Events::Event_TermOnHandled);
		}
	}

	return Handled > 0;
}
//---------------------------------------------------------------------

void CInputTranslator::UpdateTime(float ElapsedTime)
{
	for (const auto& Ctx : _Contexts)
	{
		if (!Ctx.Enabled) continue;

		if (auto pLayout = Ctx.Layout.get())
		{
			for (auto& Pair : pLayout->States)
				Pair.second->OnTimeElapsed(ElapsedTime);

			for (auto& Rec : pLayout->Events)
			{
				if (Rec.Event->OnTimeElapsed(ElapsedTime))
				{
					Events::CEvent NewEvent;
					NewEvent.ID = Rec.OutEventID;
					FireEvent(NewEvent, Events::Event_TermOnHandled);
				}
			}
		}
	}
}
//---------------------------------------------------------------------

bool CInputTranslator::CheckState(CStrID StateID) const
{
	for (const auto& Ctx : _Contexts)
	{
		if (!Ctx.Enabled) continue;

		if (auto pLayout = Ctx.Layout.get())
		{
			auto It = pLayout->States.find(StateID);
			if (It != pLayout->States.cend() && It->second->IsOn()) OK;
		}
	}

	FAIL;
}
//---------------------------------------------------------------------

void CInputTranslator::Reset(/*device type*/)
{
	for (const auto& Ctx : _Contexts)
		if (Ctx.Enabled && Ctx.Layout)
			Ctx.Layout->Reset();
}
//---------------------------------------------------------------------

}
