#include "InputTranslator.h"

#include <Input/InputEvents.h>
#include <Input/InputDevice.h>

namespace Input
{

void CInputTranslator::EnableContext(CStrID ID)
{
}
//---------------------------------------------------------------------

void CInputTranslator::DisableContext(CStrID ID)
{
}
//---------------------------------------------------------------------

void CInputTranslator::ConnectToDevice(IInputDevice* pDevice, U16 Priority)
{
	if (pDevice->GetAxisCount() > 0)
	{
		Events::PSub NewSub;
		pDevice->Subscribe(&Event::AxisMove::RTTI, this, &CInputTranslator::OnAxisMove, &NewSub);
		Subscriptions.Add(NewSub);
	}
	if (pDevice->GetButtonCount() > 0)
	{
		Events::PSub NewSub;
		pDevice->Subscribe(&Event::ButtonDown::RTTI, this, &CInputTranslator::OnButtonDown, &NewSub);
		Subscriptions.Add(NewSub);
		pDevice->Subscribe(&Event::ButtonUp::RTTI, this, &CInputTranslator::OnButtonUp, &NewSub);
		Subscriptions.Add(NewSub);
	}
}
//---------------------------------------------------------------------

void CInputTranslator::DisconnectFromDevice(const IInputDevice* pDevice)
{
	for (UPTR i = 0; i < Subscriptions.GetCount(); )
		if (Subscriptions[i]->GetDispatcher() == pDevice)
			Subscriptions.RemoveAt(i);
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