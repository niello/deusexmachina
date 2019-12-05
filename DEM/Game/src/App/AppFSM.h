#pragma once
#ifndef __DEM_L2_APP_FSM_H__
#define __DEM_L2_APP_FSM_H__

#include "StateHandler.h"
#include <Events/EventsFwd.h>

// Application state machine manages different application states and their transitions.
// Use it into your application class if app has more than one state.

namespace App
{
typedef Ptr<class CStateHandler> PStateHandler;

class CAppFSM
{
protected:

	CStrID					CurrState;
	CStrID					RequestedState;
	CArray<PStateHandler>	StateHandlers;
	CStateHandler*			pCurrStateHandler = nullptr;
	Data::PParams			TransitionParams;

	void ChangeState(CStrID NextState);

	DECLARE_EVENT_HANDLER(CloseApplication, OnCloseRequest);

public:

	CAppFSM();
	~CAppFSM();

	void			Init(CStrID InitialState);
	bool			Advance();
	void			Clear();

	void			AddStateHandler(CStateHandler* pHandler);
	void			RequestState(CStrID NewState, Data::PParams Params = nullptr);

	CStateHandler*	FindStateHandlerByID(CStrID ID) const;
	CStateHandler*	FindStateHandlerByRTTI(const Core::CRTTI& RTTI) const;
	CStateHandler*	GetStateHandlerAt(IPTR Idx) const { return StateHandlers[Idx]; }
	CStateHandler*	GetCurrentStateHandler() const { return pCurrStateHandler; }
	CStrID			GetCurrentStateID() const { return CurrState; }
};

inline void CAppFSM::RequestState(CStrID NewState, Data::PParams Params)
{
	RequestedState = NewState;
	TransitionParams = Params;
}
//---------------------------------------------------------------------

inline void CAppFSM::AddStateHandler(CStateHandler* pHandler)
{
    StateHandlers.Add(pHandler);
    pHandler->OnAttachToApplication();
}
//---------------------------------------------------------------------

inline CStateHandler* CAppFSM::FindStateHandlerByID(CStrID ID) const
{
	for (UPTR i = 0; i < StateHandlers.GetCount(); ++i)
		if (StateHandlers[i]->GetID() == ID)
			return StateHandlers[i];
	return nullptr;
}
//---------------------------------------------------------------------

inline CStateHandler* CAppFSM::FindStateHandlerByRTTI(const Core::CRTTI& RTTI) const
{
	for (UPTR i = 0; i < StateHandlers.GetCount(); ++i)
		if (StateHandlers[i]->IsExactly(RTTI))
			return StateHandlers[i];
	return nullptr;
}
//---------------------------------------------------------------------

}

#endif
