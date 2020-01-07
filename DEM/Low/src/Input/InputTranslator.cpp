#include "InputTranslator.h"

#include <Input/InputEvents.h>
#include <Input/InputDevice.h>
#include <Input/ControlLayout.h>
#include <Events/Subscription.h>
#include <Core/Application.h>

namespace Input
{

CInputTranslator::CInputTranslator(CStrID UserID)
	: _UserID(UserID)
{
	Contexts.SetKeepOrder(true);
}
//---------------------------------------------------------------------

CInputTranslator::~CInputTranslator()
{
	Clear();
}
//---------------------------------------------------------------------

void CInputTranslator::Clear()
{
	for (UPTR i = 0; i < Contexts.GetCount(); ++i)
		if (Contexts[i].pLayout) n_delete(Contexts[i].pLayout);
	Contexts.Clear();
	DeviceSubs.Clear();
	EventQueue.Clear();
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

bool CInputTranslator::UpdateParams(DEM::Core::CApplication& App, std::set<std::string>* pOutParams)
{
	auto ParamGetter = [&App, UserID = _UserID](const char* pKey) -> std::string
	{
		CString Value = App.GetStringSetting(pKey, CString::Empty, UserID);
		return Value.IsValid() ? Value.CStr() : "";
	};

	bool Result = true;

	for (UPTR i = 0; i < Contexts.GetCount(); ++i)
	{
		if (auto pLayout = Contexts[i].pLayout)
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
	for (UPTR i = 0; i < Contexts.GetCount(); ++i)
		if (Contexts[i].ID == ID) FAIL;
	CInputContext& NewCtx = *Contexts.Add();
	NewCtx.ID = ID;
	NewCtx.Enabled = false;
	NewCtx.pLayout = Bypass ? nullptr : n_new(CControlLayout);
	OK;
}
//---------------------------------------------------------------------

void CInputTranslator::DestroyContext(CStrID ID)
{
	for (UPTR i = 0; i < Contexts.GetCount(); ++i)
		if (Contexts[i].ID == ID)
		{
			n_delete(Contexts[i].pLayout);
			Contexts.RemoveAt(i);
			break;
		}
}
//---------------------------------------------------------------------

bool CInputTranslator::HasContext(CStrID ID) const
{
	for (UPTR i = 0; i < Contexts.GetCount(); ++i)
		if (Contexts[i].ID == ID)
			return true;

	return false;
}
//---------------------------------------------------------------------

CControlLayout* CInputTranslator::GetContextLayout(CStrID ID)
{
	for (UPTR i = 0; i < Contexts.GetCount(); ++i)
		if (Contexts[i].ID == ID) return Contexts[i].pLayout;
	return nullptr;
}
//---------------------------------------------------------------------

void CInputTranslator::EnableContext(CStrID ID)
{
	for (UPTR i = 0; i < Contexts.GetCount(); ++i)
		if (Contexts[i].ID == ID)
		{
			Contexts[i].Enabled = true;
			break;
		}
}
//---------------------------------------------------------------------

void CInputTranslator::DisableContext(CStrID ID)
{
	for (UPTR i = 0; i < Contexts.GetCount(); ++i)
		if (Contexts[i].ID == ID)
		{
			Contexts[i].Enabled = false;
			Contexts[i].pLayout->Reset();
			break;
		}
}
//---------------------------------------------------------------------

void CInputTranslator::ConnectToDevice(IInputDevice* pDevice, U16 Priority)
{
	if (!pDevice || IsConnectedToDevice(pDevice)) return;

	if (pDevice->GetAxisCount() > 0)
		DeviceSubs.Add(pDevice->Subscribe(&Event::AxisMove::RTTI, this, &CInputTranslator::OnAxisMove));

	if (pDevice->GetButtonCount() > 0)
	{
		DeviceSubs.Add(pDevice->Subscribe(&Event::ButtonDown::RTTI, this, &CInputTranslator::OnButtonDown));
		DeviceSubs.Add(pDevice->Subscribe(&Event::ButtonUp::RTTI, this, &CInputTranslator::OnButtonUp));
	}

	if (pDevice->CanInputText())
		DeviceSubs.Add(pDevice->Subscribe(&Event::TextInput::RTTI, this, &CInputTranslator::OnTextInput));
}
//---------------------------------------------------------------------

void CInputTranslator::DisconnectFromDevice(const IInputDevice* pDevice)
{
	for (UPTR i = 0; i < DeviceSubs.GetCount(); )
	{
		if (DeviceSubs[i]->GetDispatcher() == pDevice)
			DeviceSubs.RemoveAt(i);
		else ++i;
	}
}
//---------------------------------------------------------------------

UPTR CInputTranslator::GetConnectedDevices(CArray<IInputDevice*>& OutDevices) const
{
	const UPTR PrevCount = OutDevices.GetCount();
	for (const auto& Sub : DeviceSubs)
	{
		auto pDevice = static_cast<IInputDevice*>(Sub->GetDispatcher());
		if (!OutDevices.Contains(pDevice))
			OutDevices.Add(pDevice);
	}
	return OutDevices.GetCount() - PrevCount;
}
//---------------------------------------------------------------------

bool CInputTranslator::IsConnectedToDevice(const IInputDevice* pDevice) const
{
	for (const auto& Sub : DeviceSubs)
		if (Sub->GetDispatcher() == pDevice)
			return true;

	return false;
}
//---------------------------------------------------------------------

void CInputTranslator::TransferAllDevices(CInputTranslator* pNewOwner)
{
	if (pNewOwner)
	{
		for (const auto& Sub : DeviceSubs)
			pNewOwner->ConnectToDevice(static_cast<IInputDevice*>(Sub->GetDispatcher()));
	}

	DeviceSubs.Clear();
}
//---------------------------------------------------------------------

bool CInputTranslator::OnAxisMove(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	const Event::AxisMove& Ev = static_cast<const Event::AxisMove&>(Event);

	for (UPTR i = 0; i < Contexts.GetCount(); ++i)
	{
		CInputContext& Ctx = Contexts[i];
		if (!Ctx.Enabled) continue;

		CControlLayout* pLayout = Ctx.pLayout;
		if (pLayout)
		{
			for (auto& Pair : pLayout->States)
				Pair.second->OnAxisMove(Ev.Device, Ev);

			for (auto& Rec : pLayout->Events)
			{
				if (Rec.Event->OnAxisMove(Ev.Device, Ev))
				{
					Events::CEvent& NewEvent = *EventQueue.Add();
					NewEvent.ID = Rec.OutEventID;
					NewEvent.Params = n_new(Data::CParams(1));
					NewEvent.Params->Set<float>(CStrID("Amount"), Ev.Amount);
					OK;
				}
			}
		}
		else
		{
			// Bypass context
			Event::AxisMove BypassEvent(Ev.Device, Ev.Code, Ev.Amount, _UserID);
			if (FireEvent(BypassEvent, Events::Event_TermOnHandled) > 0) OK;
		}
	}

	FAIL;
}
//---------------------------------------------------------------------

bool CInputTranslator::OnButtonDown(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	const Event::ButtonDown& Ev = static_cast<const Event::ButtonDown&>(Event);

	for (UPTR i = 0; i < Contexts.GetCount(); ++i)
	{
		CInputContext& Ctx = Contexts[i];
		if (!Ctx.Enabled) continue;

		CControlLayout* pLayout = Ctx.pLayout;
		if (pLayout)
		{
			for (auto& Pair : pLayout->States)
				Pair.second->OnButtonDown(Ev.Device, Ev);

			for (auto& Rec : pLayout->Events)
			{
				if (Rec.Event->OnButtonDown(Ev.Device, Ev))
				{
					Events::CEvent& NewEvent = *EventQueue.Add();
					NewEvent.ID = Rec.OutEventID;
					OK;
				}
			}
		}
		else
		{
			// Bypass context
			Event::ButtonDown BypassEvent(Ev.Device, Ev.Code, _UserID);
			if (FireEvent(BypassEvent, Events::Event_TermOnHandled) > 0) OK;
		}
	}

	FAIL;
}
//---------------------------------------------------------------------

bool CInputTranslator::OnButtonUp(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	const Event::ButtonUp& Ev = static_cast<const Event::ButtonUp&>(Event);

	for (UPTR i = 0; i < Contexts.GetCount(); ++i)
	{
		CInputContext& Ctx = Contexts[i];
		if (!Ctx.Enabled) continue;

		CControlLayout* pLayout = Ctx.pLayout;
		if (pLayout)
		{
			for (auto& Pair : pLayout->States)
				Pair.second->OnButtonUp(Ev.Device, Ev);

			for (auto& Rec : pLayout->Events)
			{
				if (Rec.Event->OnButtonUp(Ev.Device, Ev))
				{
					Events::CEvent& NewEvent = *EventQueue.Add();
					NewEvent.ID = Rec.OutEventID;
					OK;
				}
			}
		}
		else
		{
			// Bypass context
			Event::ButtonUp BypassEvent(Ev.Device, Ev.Code, _UserID);
			if (FireEvent(BypassEvent, Events::Event_TermOnHandled) > 0) OK;
		}
	}

	FAIL;
}
//---------------------------------------------------------------------

bool CInputTranslator::OnTextInput(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	const Event::TextInput& Ev = static_cast<const Event::TextInput&>(Event);

	for (UPTR i = 0; i < Contexts.GetCount(); ++i)
	{
		CInputContext& Ctx = Contexts[i];
		if (!Ctx.Enabled) continue;

		CControlLayout* pLayout = Ctx.pLayout;
		if (pLayout)
		{
			for (auto& Pair : pLayout->States)
				Pair.second->OnTextInput(Ev.Device, Ev);

			for (auto& Rec : pLayout->Events)
			{
				if (Rec.Event->OnTextInput(Ev.Device, Ev))
				{
					Events::CEvent& NewEvent = *EventQueue.Add();
					NewEvent.ID = Rec.OutEventID;
					OK;
				}
			}
		}
		else
		{
			// Bypass context
			Event::TextInput BypassEvent(Ev.Device, Ev.Text, Ev.CaseSensitive, _UserID);
			if (FireEvent(BypassEvent, Events::Event_TermOnHandled) > 0) OK;
		}
	}

	FAIL;
}
//---------------------------------------------------------------------

void CInputTranslator::UpdateTime(float ElapsedTime)
{
	for (UPTR i = 0; i < Contexts.GetCount(); ++i)
	{
		CInputContext& Ctx = Contexts[i];
		if (!Ctx.Enabled) continue;

		CControlLayout* pLayout = Ctx.pLayout;

		for (auto& Pair : pLayout->States)
			Pair.second->OnTimeElapsed(ElapsedTime);

		for (auto& Rec : pLayout->Events)
		{
			if (Rec.Event->OnTimeElapsed(ElapsedTime))
			{
				Events::CEvent& NewEvent = *EventQueue.Add();
				NewEvent.ID = Rec.OutEventID;
			}
		}
	}
}
//---------------------------------------------------------------------

void CInputTranslator::FireQueuedEvents(/*max count*/)
{
	for (UPTR i = 0; i < EventQueue.GetCount(); ++i)
		FireEvent(EventQueue[i]);
	EventQueue.Clear();
}
//---------------------------------------------------------------------

bool CInputTranslator::CheckState(CStrID StateID) const
{
	for (UPTR i = 0; i < Contexts.GetCount(); ++i)
	{
		const CInputContext& Ctx = Contexts[i];
		if (!Ctx.Enabled) continue;

		auto It = Ctx.pLayout->States.find(StateID);
		if (It != Ctx.pLayout->States.cend() && It->second->IsOn()) OK;
	}

	FAIL;
}
//---------------------------------------------------------------------

void CInputTranslator::Reset(/*device type*/)
{
	for (UPTR i = 0; i < Contexts.GetCount(); ++i)
		if (Contexts[i].Enabled)
			Contexts[i].pLayout->Reset();
}
//---------------------------------------------------------------------

}