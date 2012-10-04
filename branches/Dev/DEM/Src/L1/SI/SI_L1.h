#pragma once
#ifndef __DEM_L1_SI_H__
#define __DEM_L1_SI_H__

// Script interface for core engine L1 subsystems
// Use it to register SI of subsystems in main application module

namespace SI
{
	bool RegisterGlobals();
	bool RegisterEventManager();
	bool RegisterTimeServer();
}

#endif
