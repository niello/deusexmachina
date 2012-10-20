#pragma once
#ifndef __DEM_L2_SI_H__
#define __DEM_L2_SI_H__

// Script interface for engine L2 subsystems
// Use it to register SI of subsystems in main application module

namespace SI
{
	bool RegisterScriptObjectSIEx();
	bool RegisterEntityManager();
	bool RegisterNavMesh();
}

#endif
