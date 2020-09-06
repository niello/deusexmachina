#pragma once
#include <Resources/Resource.h>
#include <Data/Metadata.h>
#include <Data/StringID.h>

// Smart object instance that can switch between different states and offer available interactions

namespace DEM::Game
{

struct CSmartObjectComponent
{
	//DEM::Anim::CTimelinePlayer Player;
	//DEM::Anim::PTimelineTrack  Track; // for the last instantiated TL (best setup is the same TL asset for all states & transitions)

	Resources::PResource Asset; // CSmartObject
	CStrID AssetID; //???or PResource? ID and object inside, nicely wrapped
	// curr state
	// next state
	//???transition progress / current state timer (one multipurpose time float)? or inside timeline player?
	//!!!NB: if in player, must save prev time, not curr, or some TL part may be skipped on game reload!

	CStrID GetAssetID() const { return Asset ? Asset->GetUID() : CStrID::Empty; }

	// get current time - from player (transition time / full time in state since enter)
	// get duration - transition duration / one state loop duration
	// is in transition
};

}

namespace DEM::Meta
{

template<> inline constexpr auto RegisterClassName<Game::CSmartObjectComponent>() { return "DEM::Game::CSmartObjectComponent"; }
template<> inline constexpr auto RegisterMembers<Game::CSmartObjectComponent>()
{
	return std::make_tuple
	(
		Member<Game::CSmartObjectComponent, CStrID>(1, "AssetID", &Game::CSmartObjectComponent::GetAssetID, &Game::CSmartObjectComponent::AssetID)
	);
}

}
