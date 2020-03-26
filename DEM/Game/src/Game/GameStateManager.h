#pragma once
#include <Data/Ptr.h>
#include <Core/RTTI.h>

// Stack FSM that controls a game world behaviour and interaction.
// Current state is on the top of the stack.

namespace DEM::Game
{
typedef Ptr<class CGameState> PGameState;

class CGameStateManager final
{
public:

	void PushState(PGameState NewState);
	void PopState();
	void PopStateTo(const ::Core::CRTTI& StateType);
	void PopStateTo(PGameState State);
	void PopAllStates();
};

}
