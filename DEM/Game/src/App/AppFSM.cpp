#include "AppFSM.h"

#include <Events/EventServer.h>

namespace App
{
CAppFSM::CAppFSM() {}
CAppFSM::~CAppFSM() {}

void CAppFSM::Init(CStrID InitialState)
{
	SUBSCRIBE_PEVENT(CloseApplication, CAppFSM, OnCloseRequest);
	ChangeState(InitialState);
}
//---------------------------------------------------------------------
	
bool CAppFSM::Advance()
{
	if (CurrState == CStrID::Empty) FAIL;

	CStrID NewState = pCurrStateHandler->OnFrame();

	if (RequestedState != CurrState) ChangeState(RequestedState);
	else if (NewState != CurrState) ChangeState(NewState);

	Sys::Sleep(0);

	OK;
}
//---------------------------------------------------------------------

void CAppFSM::Clear()
{
	UNSUBSCRIBE_EVENT(CloseApplication);

	CurrState = CStrID::Empty;
	RequestedState = CurrState;
	pCurrStateHandler = nullptr;
	TransitionParams = nullptr;

	for (UPTR i = 0; i < StateHandlers.size(); ++i)
		StateHandlers[i]->OnRemoveFromApplication();
	StateHandlers.clear();
}
//---------------------------------------------------------------------

void CAppFSM::ChangeState(CStrID NextState)
{
	if (pCurrStateHandler) pCurrStateHandler->OnStateLeave(NextState);

	CStrID PrevState = CurrState;
	CurrState = NextState;
	if (NextState.IsValid())
	{
		pCurrStateHandler = FindStateHandlerByID(NextState);
		if (pCurrStateHandler) pCurrStateHandler->OnStateEnter(PrevState, TransitionParams);
		else n_assert(CurrState == CStrID::Empty);
	}
	RequestedState = CurrState;
	//???TransitionParams = nullptr;?
}
//---------------------------------------------------------------------

bool CAppFSM::OnCloseRequest(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	RequestState(CStrID::Empty);
	OK;
}
//---------------------------------------------------------------------

}
