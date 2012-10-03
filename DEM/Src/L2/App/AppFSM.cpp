#include "AppFSM.h"

#include <Events/EventManager.h>

namespace App
{

void CAppFSM::Init(CStrID InitialState)
{
	SUBSCRIBE_PEVENT(CloseApplication, CAppFSM, OnCloseRequest);
	ChangeState(InitialState);
}
//---------------------------------------------------------------------
	
bool CAppFSM::Advance()
{
	if (CurrState == APP_STATE_EXIT) FAIL;

	CStrID NewState = CurrStateHandler->OnFrame();

	if (RequestedState.IsValid()) ChangeState(RequestedState);
	else if (NewState != CurrStateHandler->GetID()) ChangeState(NewState);

	n_sleep(0.0);

	OK;
}
//---------------------------------------------------------------------

void CAppFSM::Clear()
{
	UNSUBSCRIBE_EVENT(CloseApplication);

	CurrState = CStrID::Empty;
	RequestedState = CStrID::Empty;
	CurrStateHandler = NULL;
	TransitionParams = NULL;

	for (int i = 0; i < StateHandlers.Size(); i++)
		StateHandlers[i]->OnRemoveFromApplication();
	StateHandlers.Clear();
}
//---------------------------------------------------------------------

void CAppFSM::ChangeState(CStrID NextState)
{
	if (CurrStateHandler) CurrStateHandler->OnStateLeave(NextState);

	CStrID PrevState = CurrState;
	CurrState = NextState;
	if (NextState.IsValid())
	{
		CurrStateHandler = FindStateHandlerByID(NextState);
		if (CurrStateHandler) CurrStateHandler->OnStateEnter(PrevState, TransitionParams);
		else n_assert(CurrState == APP_STATE_EXIT);
	}
	RequestedState = CStrID::Empty;
	//???TransitionParams = NULL;?
}
//---------------------------------------------------------------------

bool CAppFSM::OnCloseRequest(const Events::CEventBase& Event)
{
	RequestState(APP_STATE_EXIT);
	OK;
}
//---------------------------------------------------------------------

}