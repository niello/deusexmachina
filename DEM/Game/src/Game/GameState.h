#pragma once
#include <Core/Object.h>

// Game state encapsulates logic, UI and input processing for a certain game mode.
// All game states live on top of the game world and describe how it lives.
// Game state examples: exploration, cutscene, battle, dialogue etc.

namespace DEM::Game
{
typedef Ptr<class CGameState> PGameState;

class CGameState : public ::Core::CObject
{
	RTTI_CLASS_DECL;

public:

	virtual void        OnEnter(CGameState* pFromState) {}
	virtual void        OnExit(CGameState* pToState) {}
	virtual CGameState* Update(double FrameTime) { return this; }
};

}
