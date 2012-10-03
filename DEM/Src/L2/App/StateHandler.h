#pragma once
#ifndef __IPG_APP_STATE_HANDLER_H__
#define __IPG_APP_STATE_HANDLER_H__

#include <Core/RefCounted.h>
#include <Data/Params.h>

// State handlers implement actual application state behavior in subclasses
// of Application::CStateHandler. The Application class calls state handler
// objects when a new state is entered, when the current state is left, and
// for each frame.
// State handlers must implement the OnStateEnter(), OnStateLeave() and
// OnFrame() methods accordingly.
// Based on mangalore StateHandler (C) 2003 RadonLabs GmbH

namespace App
{
using namespace Data;

class CStateHandler: public Core::CRefCounted
{
	DeclareRTTI;

protected:

	CStrID ID;

public:

	CStateHandler(CStrID StateID) { n_assert(StateID.IsValid()); ID = StateID; }
	virtual ~CStateHandler() {}

	virtual void	OnAttachToApplication() {}
	virtual void	OnRemoveFromApplication() {}
	virtual void	OnStateEnter(CStrID PrevState, PParams Params = NULL) {}
	virtual void	OnStateLeave(CStrID NextState) {}
	virtual CStrID	OnFrame() { return CStrID::Empty; }

	CStrID			GetID() const { return ID; }
};

}

#endif
