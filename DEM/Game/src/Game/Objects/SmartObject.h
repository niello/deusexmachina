#pragma once
#include <Resources/Resource.h>
#include <sol/forward.hpp>
#include <vector>

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
	ETransitionInterruptionMode InterruptionMode;
};

struct CSmartObjectStateInfo
{
	CStrID        ID;
	CTimelineTask TimelineTask;

	// state logic object ptr (optional)

	std::vector<CSmartObjectTransitionInfo> Transitions;
};

class CSmartObject : public Resources::CResourceObject
{
	RTTI_CLASS_DECL;

protected:

	CStrID                             _ID;
	CStrID                             _DefaultState;
	std::string                        _ScriptSource;
	std::vector<CSmartObjectStateInfo> _States;
	std::vector<CStrID>                _Interactions;

public:

	CSmartObject(CStrID ID, CStrID DefaultState, std::string_view ScriptSource);

	virtual bool IsResourceValid() const override { return !_States.empty(); }

	// TODO: Add* logic to constructor? Or for runtime changes need also Remove*.
	bool          AddState(CStrID ID, CTimelineTask&& TimelineTask/*, state logic object ptr (optional)*/);
	bool          AddTransition(CStrID FromID, CStrID ToID, CTimelineTask&& TimelineTask);
	bool          AddInteraction(CStrID ID);
	bool          InitScript(sol::state& Lua);

	const CSmartObjectStateInfo*      FindState(CStrID ID) const;
	const CSmartObjectTransitionInfo* FindTransition(CStrID FromID, CStrID ToID) const;

	sol::function GetScriptFunction(sol::state& Lua, std::string_view Name) const;
	CStrID        GetID() const { return _ID; }
	CStrID        GetDefaultState() const { return _DefaultState; }
	const auto&   GetScriptSource() const { return _ScriptSource; }
	const auto&   GetInteractions() const { return _Interactions; }
};

}
