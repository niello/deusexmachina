#pragma once
#ifndef __DEM_L2_GAME_MANAGER_H__
#define __DEM_L2_GAME_MANAGER_H__

#include <Core/RefCounted.h>

// Managers are Singleton objects which care about some "specific global
// stuff". They should be subclassed by Mangalore applications to implement
// globals aspects of the application (mainly global game play related stuff).
//
// Managers are created directly through Factory<> and attached to the
// Game::Server object which will call several predefined virtual methods
// which are overridden by the subclasses.
//
// Standard-Mangalore uses several Managers to offer timing information
// (CTimeServer), create entities and properties (CEntityFactory), manage
// game entities (CEntityManager) and so forth.
//
// Managers are derived from Message::Port, so you *can* optionally use them to
// receive and process messages.
//
// Based on mangalore Game::Manager_(C) 2005 Radon Labs GmbH

namespace Game
{

class CManager: public Core::CRefCounted
{
	DeclareRTTI;

protected:

	bool _IsActive;

public:

	CManager();
	virtual ~CManager() = 0;

	virtual void	Activate();
	virtual void	Deactivate();
	bool			IsActive() const { return _IsActive; }
};

}

#endif

