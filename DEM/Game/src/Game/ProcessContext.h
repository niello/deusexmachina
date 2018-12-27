#pragma once
#ifndef __DEM_L2_GAME_PROCESS_CONTEXT_H__
#define __DEM_L2_GAME_PROCESS_CONTEXT_H__

#include <Game/ActionContext.h>

// Process context extends an action context with the process state

namespace Game
{

enum EProcessState
{
	Process_NotStarted,
	Process_InProgress,
	Process_Done,
	Process_Stopped
};

class CProcessContext
{
protected:

	//subscriptions and event handler

public:

	CActionContext	ActionContext;
	EProcessState	State;
};

}

#endif
