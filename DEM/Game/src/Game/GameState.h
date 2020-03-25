#pragma once
#include <Core/Object.h>

// ...

namespace DEM::Game
{

class CGameState //: public ::Core::CObject
{
	//RTTI_CLASS_DECL;

public:

	//CApplicationState(CApplication& Application) : App(Application) {}

	virtual void		OnEnter(CGameState* pFromState) {}
	virtual void		OnExit(CGameState* pToState) {}
	virtual CGameState*	Update(double FrameTime) { return this; }
};

}
