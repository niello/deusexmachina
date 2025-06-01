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
protected:

	std::vector<PGameState> _Stack;

public:

	CGameStateManager();
	~CGameStateManager();

	void        Update(double FrameTime);

	void        PushState(PGameState NewState);
	PGameState  PopState();
	void        PopStateTo(const Core::CRTTI& StateType);
	void        PopStateTo(const CGameState& State);
	void        PopAllStates();

	CGameState* FindState(const Core::CRTTI& StateType);
	UPTR        GetStateStackDepth() const { return _Stack.size(); }
};

}
