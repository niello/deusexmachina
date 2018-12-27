#pragma once
#ifndef __DEM_L2_GAME_PROCESS_H__
#define __DEM_L2_GAME_PROCESS_H__

#include <Game/Action.h>

// Process is a kind of action that lasts in time. It can start, progress and stop.
// Unlike an immediate action, process must be managed by some external system that
// controls its state.
// DEM action process model supports one of the following scenarios:
// Start - {Progress} - Done - End
// Start - {Progress} - Abort

namespace Game
{
class CProcessContext;

class CProcess: public IAction
{
public:

	virtual bool	Execute(const CActionContext& Context) const;
	//bool Update();
	//void Stop();

	virtual bool	OnStart(const CProcessContext& Context) const = 0;
	//virtual bool	OnProgress(const CProcessContext& Context) const = 0;
	virtual bool	OnDone(const CProcessContext& Context) const = 0;
	virtual bool	OnEnd(const CProcessContext& Context) const = 0;
	virtual bool	OnAbort(const CProcessContext& Context) const = 0;
};

}

#endif
