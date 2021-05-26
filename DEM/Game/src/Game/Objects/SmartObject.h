#pragma once
#include <Animation/TimelineTask.h>
#include <Game/Interaction/Zone.h>
#include <sol/forward.hpp>
#include <vector>
#include <map>

// Smart object asset describes a set of states, transitions between them,
// and interactions available over the object under different conditions.
// This asset is stateless, state is stored in CSmartObjectComponent.

// NB: vectors are sorted by ID where possible

//!!!FIXME: separate smart object and FSM?!

namespace DEM::Game
{
using PSmartObject = Ptr<class CSmartObject>;
class CGameSession;

enum class ETransitionInterruptionMode : U8
{
	ResetToStart, // Current transition is aborted back to the outgoing state, then new transition starts
	RewindToEnd,  // Current transition is played to finish instantly, then new transition starts
	Proportional, // Current transition progress % is carried to the new transition, inverted if it is inverse of the current one
	Forbid,       // Current transition is kept, request is discarded
	Force,        // Cancel current transition and force-set requested state
	Wait          // Wait for current transition to end and then start requested one
};

struct CSmartObjectTransitionInfo
{
	CStrID                      TargetStateID;
	Anim::CTimelineTask         TimelineTask;
	ETransitionInterruptionMode InterruptionMode = ETransitionInterruptionMode::ResetToStart;
};

struct CSmartObjectStateInfo
{
	CStrID              ID;
	Anim::CTimelineTask TimelineTask;

	// state logic object ptr (optional)

	std::vector<CSmartObjectTransitionInfo> Transitions;
};

class CSmartObject : public ::Core::CObject
{
	RTTI_CLASS_DECL(DEM::Game::CSmartObject, ::Core::CObject);

protected:

	CStrID                                _ID;
	CStrID                                _DefaultState;
	std::string                           _ScriptSource;
	std::vector<CSmartObjectStateInfo>    _States;
	std::vector<CZone>                    _InteractionZones;
	std::map<CStrID, CFixedArray<CStrID>> _InteractionOverrides;

public:

	CSmartObject(CStrID ID, CStrID DefaultState, std::string_view ScriptSource,
		std::vector<CSmartObjectStateInfo>&& States, std::vector<CZone>&& Zones,
		std::map<CStrID, CFixedArray<CStrID>>&& InteractionOverrides);

	bool                              InitInSession(CGameSession& Session) const;

	const CSmartObjectStateInfo*      FindState(CStrID ID) const;
	const CSmartObjectTransitionInfo* FindTransition(CStrID FromID, CStrID ToID) const;

	auto                              GetInteractionZoneCount() const { return _InteractionZones.size(); }
	const auto&                       GetInteractionZone(U8 ZoneIdx) const { return _InteractionZones[ZoneIdx]; }
	const CFixedArray<CStrID>*        GetInteractionOverrides(CStrID ID) const;

	sol::function                     GetScriptFunction(sol::state& Lua, std::string_view Name) const;
	CStrID                            GetID() const { return _ID; }
	CStrID                            GetDefaultState() const { return _DefaultState; }
};

}
