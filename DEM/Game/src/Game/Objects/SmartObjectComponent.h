#pragma once
#include <Resources/Resource.h>
#include <Animation/TimelinePlayer.h>
#include <Animation/TimelineTrack.h> // For inlined destructor CSmartObjectComponent -> CTimelinePlayer
#include <Events/Signal.h>
#include <Data/Metadata.h>
#include <Scripting/SolGame.h>

// Smart object instance that can switch between different states and offer available interactions

namespace DEM::Game
{

struct CSmartObjectComponent
{
	DEM::Anim::CTimelinePlayer Player;

	Resources::PResource       Asset; // CSmartObject
	CStrID                     AssetID; // FIXME: PResource Asset must be enough, but how to deserialize without ResMgr?

	CStrID                     CurrState;
	CStrID                     NextState;
	CStrID                     RequestedState;
	bool                       Force = false; // Force-set requested state immediately?

	sol::function              UpdateScript; // Cache for faster per-frame access

	// Args are: our entity, state before, state after
	Events::CSignal<void(HEntity, CStrID, CStrID)> OnTransitionStart;
	Events::CSignal<void(HEntity, CStrID, CStrID)> OnTransitionEnd;
	Events::CSignal<void(HEntity, CStrID, CStrID)> OnTransitionCancel;
	Events::CSignal<void(HEntity, CStrID, CStrID)> OnStateForceSet;

	//???transition progress / current state timer (one multipurpose time float)? or inside timeline player?
	//!!!NB: if time is in TL in player, must save/return prev time, not curr, or some TL part may be skipped on game reload!

	//CStrID GetAssetID() const { return Asset ? Asset->GetUID() : CStrID::Empty; }

	// get current time - from player (transition time / full time in state since enter)
	// get duration - transition duration / one state loop duration
};

}

namespace DEM::Meta
{

template<> inline constexpr auto RegisterClassName<Game::CSmartObjectComponent>() { return "DEM::Game::CSmartObjectComponent"; }
template<> inline constexpr auto RegisterMembers<Game::CSmartObjectComponent>()
{
	return std::make_tuple
	(
		DEM_META_MEMBER_FIELD(Game::CSmartObjectComponent, 1, AssetID),
		DEM_META_MEMBER_FIELD(Game::CSmartObjectComponent, 2, CurrState)
	);
}

}
