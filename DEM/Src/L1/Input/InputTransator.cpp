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
	FAIL;
}
//---------------------------------------------------------------------

bool CInputTranslator::OnButtonDown(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	FAIL;
}
//---------------------------------------------------------------------

bool CInputTranslator::OnButtonUp(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	FAIL;
}
//---------------------------------------------------------------------

}