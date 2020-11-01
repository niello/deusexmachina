#pragma once
#include <Resources/Resource.h>
#include <Data/FixedArray.h>
#include <Math/Vector3.h>
#include <sol/forward.hpp>
#include <vector>
#include <optional>

// Smart object asset describes a set of states, transitions between them,
// and interactions available over the object under different conditions.
// This asset is stateless, state is stored in CSmartObjectComponent.

// NB: vectors are sorted by ID where possible

namespace DEM::Game
{
using PSmartObject = Ptr<class CSmartObject>;

//???to timeline player module?
struct CTimelineTask
{
	Resources::PResource Timeline; // nullptr to disable task
	float                Speed = 1.f;
	float                StartTime = 0.f;
	float                EndTime = 1.f; //???use relative time here?
	U32                  LoopCount = 0;

	//!!!FIXME: need output map (pose, event, sound etc...), not hardcoded pose output!
	// Or predefined output params, like all poses to some root path, all events to this entity's dispatcher component etc?
	std::string          SkeletonRootRelPath;
};

enum class ETransitionInterruptionMode : U8
{
	ResetToStart,
	RewindToEnd,
	Proportional,
	Forbid
	// Force?
};

struct CSmartObjectTransitionInfo
{
	CStrID                      TargetStateID;
	CTimelineTask               TimelineTask;
	ETransitionInterruptionMode InterruptionMode = ETransitionInterruptionMode::ResetToStart;
};

struct CSmartObjectStateInfo
{
	CStrID        ID;
	CTimelineTask TimelineTask;

	// state logic object ptr (optional)

	std::vector<CSmartObjectTransitionInfo> Transitions;
};

enum class EFacingMode : U8
{
	None = 0,
	Direction,
	Point
	// TODO: SceneNode?
};

struct CSmartObjectInteractionInfo
{
	CStrID      ID;
	EFacingMode FacingMode;
	vector3     FacingDir;

	// actor anim
	// actor state
};

struct CInteractionZone
{
	CFixedArray<CSmartObjectInteractionInfo> Interactions;
	CFixedArray<vector3>                     Vertices;
	float                                    Radius = 0.f;
	bool                                     ClosedPolygon = false;
};

class CSmartObject : public Resources::CResourceObject
{
	RTTI_CLASS_DECL(DEM::Game::CSmartObject, Resources::CResourceObject);

public:

	static inline constexpr UPTR MAX_ZONES = 16; // Because only 16 bits are reserved in SwitchSmartObjectState

protected:

	CStrID                             _ID;
	CStrID                             _DefaultState;
	std::string                        _ScriptSource;
	std::vector<CSmartObjectStateInfo> _States;
	std::vector<CInteractionZone>      _InteractionZones;
	CFixedArray<CStrID>                _Interactions;
	bool                               _Static = false; // true - interaction params never change over time

public:

	CSmartObject(CStrID ID, CStrID DefaultState, bool Static, std::string_view ScriptSource,
		std::vector<CSmartObjectStateInfo>&& States, std::vector<CInteractionZone>&& InteractionZones);

	virtual bool IsResourceValid() const override { return !_States.empty(); }

	bool          InitScript(sol::state& Lua);

	const CSmartObjectStateInfo*      FindState(CStrID ID) const;
	const CSmartObjectTransitionInfo* FindTransition(CStrID FromID, CStrID ToID) const;

	bool          HasInteraction(CStrID ID) const;
	auto          GetInteractionZoneCount() const { return _InteractionZones.size(); }
	const auto&   GetInteractionZone(U8 ZoneIdx) const { return _InteractionZones[ZoneIdx]; }

	sol::function GetScriptFunction(sol::state& Lua, std::string_view Name) const;
	CStrID        GetID() const { return _ID; }
	CStrID        GetDefaultState() const { return _DefaultState; }
	bool          IsStatic() const { return _Static; }
	const auto&   GetScriptSource() const { return _ScriptSource; }
	const auto&   GetInteractions() const { return _Interactions; }
};

}
