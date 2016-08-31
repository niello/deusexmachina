#include "InputTranslator.h"

#include <Input/InputEvents.h>
#include <Input/InputDevice.h>
#include <Input/ControlLayout.h>
#include <Events/Subscription.h>
#include <Data/DataArray.h>

namespace Input
{

void CInputTranslator::Clear()
{
	for (UPTR i = 0; i < Contexts.GetCount(); ++i)
		if (Contexts[i].pLayout) n_delete(Contexts[i].pLayout);
	Contexts.Clear();

	DeviceSubs.Clear();

	//!!!clear event queue!
}
//---------------------------------------------------------------------

bool CInputTranslator::LoadSettings(const Data::CParams& Desc)
{
	Clear();

	Data::PParams ContextsDesc;
	if (Desc.Get(ContextsDesc, CStrID("Contexts")))
	{
		for (UPTR i = 0; i < ContextsDesc->GetCount(); ++i)
		{
			const Data::CParam& Prm = ContextsDesc->Get(i);
			const Data::CDataArray& ContextLayoutDesc = *Prm.GetValue<Data::PDataArray>().GetUnsafe();

			CStrID ContextID = Prm.GetName();
			if (!CreateContext(ContextID)) FAIL;
			CControlLayout* pLayout = GetContextLayout(ContextID);
			if (!pLayout || !pLayout->Initialize(ContextLayoutDesc)) FAIL;
		}
	}

	// read enabled contexts and enable them
	// read axis inversion
	// read other settings

	OK;
}
//---------------------------------------------------------------------

bool CInputTranslator::SaveSettings(Data::CParams& OutDesc) const
{
	FAIL;
}
//---------------------------------------------------------------------

bool CInputTranslator::CreateContext(CStrID ID)
{
	for (UPTR i = 0; i < Contexts.GetCount(); ++i)
		if (Contexts[i].ID == ID) FAIL;
	CInputContext& NewCtx = *Contexts.Add();
	NewCtx.ID = ID;
	NewCtx.Enabled = false;
	NewCtx.pLayout = n_new(CControlLayout);
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

CControlLayout* CInputTranslator::GetContextLayout(CStrID ID)
{
	for (UPTR i = 0; i < Contexts.GetCount(); ++i)
		if (Contexts[i].ID == ID) return Contexts[i].pLayout;
	return NULL;
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
			break;
		}
}
//---------------------------------------------------------------------

void CInputTranslator::ConnectToDevice(IInputDevice* pDevice, U16 Priority)
{
	if (pDevice->GetAxisCount() > 0)
	{
		Events::PSub NewSub;
		pDevice->Subscribe(&Event::AxisMove::RTTI, this, &CInputTranslator::OnAxisMove, &NewSub);
		DeviceSubs.Add(NewSub);
	}
	if (pDevice->GetButtonCount() > 0)
	{
		Events::PSub NewSub;
		pDevice->Subscribe(&Event::ButtonDown::RTTI, this, &CInputTranslator::OnButtonDown, &NewSub);
		DeviceSubs.Add(NewSub);
		pDevice->Subscribe(&Event::ButtonUp::RTTI, this, &CInputTranslator::OnButtonUp, &NewSub);
		DeviceSubs.Add(NewSub);
	}
}
//---------------------------------------------------------------------

void CInputTranslator::DisconnectFromDevice(const IInputDevice* pDevice)
{
	for (UPTR i = 0; i < DeviceSubs.GetCount(); )
		if (DeviceSubs[i]->GetDispatcher() == pDevice)
			DeviceSubs.RemoveAt(i);
		else ++i;
}
//---------------------------------------------------------------------

bool CInputTranslator::OnAxisMove(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	IInputDevice* pDevice = (IInputDevice*)pDispatcher;
	const Event::AxisMove& Ev = (const Event::AxisMove&)Event;

	//for each active context
	//  for each mapping in a context
	//    evaluate mapping condition
	//    if true
	//      fill output event with axis movement amount
	//      //???apply axis inversion here? or device property?
	//      queue output event and consume input event (break)

	//!!!can also "queue" states for latter polling, like pInputTranslator->CheckState("CameraRotateV")!
	//very simple to implement, difference is right here, fire event or store string -> bool (or axis amount) state

	FAIL;
}
//---------------------------------------------------------------------

bool CInputTranslator::OnButtonDown(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	IInputDevice* pDevice = (IInputDevice*)pDispatcher;
	const Event::ButtonDown& Ev = (const Event::ButtonDown&)Event;

	//for each active context
	//  for each mapping in a context
	//    evaluate mapping condition
	//    if true queue output event and consume input event (break)

	FAIL;
}
//---------------------------------------------------------------------

bool CInputTranslator::OnButtonUp(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	IInputDevice* pDevice = (IInputDevice*)pDispatcher;
	const Event::ButtonUp& Ev = (const Event::ButtonUp&)Event;

	//for each active context
	//  for each mapping in a context
	//    evaluate mapping condition
	//    if true queue output event and consume input event (break)

	FAIL;
}
//---------------------------------------------------------------------

}