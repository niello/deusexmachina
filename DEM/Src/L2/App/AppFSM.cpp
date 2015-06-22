#include "AppFSM.h"

#include <Events/EventServer.h>

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
	if (CurrState == CStrID::Empty) FAIL;

	CStrID NewState = CurrStateHandler->OnFrame();

	if (RequestedState.IsValid()) ChangeState(RequestedState);
	else if (NewState != CurrStateHandler->GetID()) ChangeState(NewState);

	Sys::Sleep(0);

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

	for (int i = 0; i < StateHandlers.GetCount(); i++)
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
		else n_assert(CurrState == CStrID::Empty);
	}
	RequestedState = CStrID::Empty;
	//???TransitionParams = NULL;?
}
//---------------------------------------------------------------------

bool CAppFSM::OnCloseRequest(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	RequestState(CStrID::Empty);
	OK;
}
//---------------------------------------------------------------------

}