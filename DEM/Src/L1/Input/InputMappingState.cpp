#include "InputMappingState.h"

#include <Input/InputServer.h>
#include <Events/EventServer.h>
#include <Data/Params.h>

namespace Input
{

CInputMappingState::CInputMappingState(): CheckInOrder(false), SendStateChangeEvent(true), State(false)
{
}
//---------------------------------------------------------------------

bool CInputMappingState::Init(CStrID Name, const Data::CParams& Desc)
{
	//!!!WRITE IT!
	OK;
}
//---------------------------------------------------------------------

void CInputMappingState::Enable()
{
	if (!IS_SUBSCRIBED(OnInputUpdated))
		DISP_SUBSCRIBE_PEVENT(InputSrv, OnInputUpdated, CInputMappingState, OnInputUpdated);
}
//---------------------------------------------------------------------

void CInputMappingState::Disable()
{
	UNSUBSCRIBE_EVENT(OnInputUpdated);
}
//---------------------------------------------------------------------

bool CInputMappingState::OnInputUpdated(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	bool OldState = State;
	State = true;
	for (CArray<CCondition>::CIterator It = Conditions.Begin(); It != Conditions.End(); ++It)
	{
		bool CndState;
		switch (It->Type)
		{
			case CT_Key:		CndState = InputSrv->CheckKeyState(It->Key, It->KeyBtnState); break;
			case CT_MouseBtn:	CndState = InputSrv->CheckMouseBtnState(It->MouseBtn, It->KeyBtnState); break;
			case CT_Wheel:		CndState = (It->WheelFwd) ? InputSrv->GetWheelForward() > 0 : InputSrv->GetWheelBackward() > 0; break;
			default:			CndState = false;
		}

		It->State = CndState;

		State &= CndState;

		if (!CndState && CheckInOrder)
		{
			for (CArray<CCondition>::CIterator It2 = Conditions.Begin(); It2 != It; ++It2)
				It2->State = false;
			break;
		}
	}

	if (OldState != State && SendStateChangeEvent)
		EventSrv->FireEvent(CStrID((State) ? EventOn : EventOff));

	FAIL;
}
//---------------------------------------------------------------------

}