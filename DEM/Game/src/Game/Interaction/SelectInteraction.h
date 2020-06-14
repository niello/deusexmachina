#pragma once
#include <Game/Interaction/Interaction.h>

// Selects an actor for the current interaction context

namespace DEM::Game
{

class CSelectInteraction : public CInteraction
{
public:

	virtual bool Execute(bool Enqueue) const override
	{
		//
		return true;
	}
};

}
