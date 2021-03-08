#pragma once
#include <Game/Interaction/Interaction.h>
#include <Data/StringID.h>

// Executes an interaction defined in a smart object

namespace DEM::Game
{

class CSmartObjectInteraction : public CInteraction
{
protected:

	CStrID _InteractionID; //???!!!is always the same as _Name?

public:

	//!!!TODO: get cursor ID from SO interaction desc or even from Lua!
	CSmartObjectInteraction(CStrID InteractionID, std::string_view CursorImage = {});

	virtual bool Execute(CInteractionContext& Context, bool Enqueue) const override;
};

}
