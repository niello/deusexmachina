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

class SwitchSmartObjectState : public Events::CEventNative
{
	NATIVE_EVENT_DECL(SwitchSmartObjectState, Events::CEventNative);

public:

	HEntity              Object;
	vector3              Pos;
	std::optional<float> FaceDir;
	CStrID               ObjectState;
	bool                 ForceObjectState = false; // Force-set requested state immediately?
	// point, face direction min & max (angle?), actor animation and/or state, object entity handle, object target state, bool force

	explicit SwitchSmartObjectState(/*const vector3& Destination, float Speed*/)/* : _Destination(Destination), _Speed(Speed)*/ {}
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
