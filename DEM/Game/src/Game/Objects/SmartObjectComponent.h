#pragma once
#include <Resources/Resource.h>
#include <Animation/TimelinePlayer.h>
#include <Animation/TimelineTrack.h> // For inlined destructor CSmartObjectComponent -> CTimelinePlayer
#include <Events/EventNative.h>
#include <Data/Metadata.h>
#include <Data/StringID.h>
#include <sol/sol.hpp>

// Smart object instance that can switch between different states and offer available interactions

namespace DEM::Game
{

// NB: avoid unnecessary Push[OrUpdate]Child to preserve cached values
class SwitchSmartObjectState : public Events::CEventNative
{
	NATIVE_EVENT_DECL(SwitchSmartObjectState, Events::CEventNative);

public:

	CStrID  _Interaction;
	CStrID  _State;
	HEntity _Object;
	U16     _AllowedZones = 0;    // Cache. Bit flags for up to 16 zones.
	U8      _ZoneIndex = 0;       // Cache
	bool    _Force = false;       // true - force-set requested state immediately
	bool    _PathScanned = false; // Cache

	explicit SwitchSmartObjectState(CStrID Interaction, HEntity Object, CStrID State, bool Force) : _Interaction(Interaction), _Object(Object), _State(State), _Force(Force) {}
};

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
		Member<Game::CSmartObjectComponent, CStrID>(1, "AssetID", &Game::CSmartObjectComponent::AssetID, &Game::CSmartObjectComponent::AssetID)
	);
}

}
