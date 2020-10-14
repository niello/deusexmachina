#pragma once
#include <Core/Object.h>

// Game state encapsulates logic, UI and input processing for a certain game mode.
// All game states live on top of the game world and describe how it lives.
// Game state examples: exploration, cutscene, battle, dialogue etc.
// Game states can be nested. Only the topmost state is active but all underlying
// states are kept alive and are inactive until all their nested states are popped.

namespace DEM::Game
{
typedef Ptr<class CGameState> PGameState;

class CGameState : public ::Core::CObject
{
	RTTI_CLASS_DECL(DEM::Game::CGameState, ::Core::CObject);

public:

	virtual void        OnEnter(CGameState* pFromState) {}
	virtual void        OnExit(CGameState* pToState) {}
	virtual void        OnNestedStatePushed(CGameState* pNestedState) {}
	virtual void        OnNestedStatePopping(CGameState* pNestedState) {}
	virtual CGameState* Update(double FrameTime) { return this; }
};

}
