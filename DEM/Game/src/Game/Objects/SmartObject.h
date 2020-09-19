#pragma once
#include <Resources/Resource.h>
#include <sol/sol.hpp>
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
};

class CSmartObject : public Resources::CResourceObject
{
	RTTI_CLASS_DECL;

protected:

	//transitions inside states? no custom logic = scripted OnEnter etc always?
	//!!!instance can cache OnEnter etc functions! or not instance, but class?
	struct CStateRecord
	{
		CStrID        ID;
		CTimelineTask TimelineTask;

		// state logic object ptr (optional)

		std::vector<std::pair<CStrID, CTimelineTask>> Transitions;
	};

	CStrID           _ID;
	CStrID           _DefaultState;
	std::string      _ScriptSource; //???CStrID of ScriptObject resource? Loader will require Lua.

	//sol::environment _ScriptObject; //???or store as sol::table? what is the difference in size?
	sol::function    _OnStateEnter;
	sol::function    _OnStateStartEntering;
	sol::function    _OnStateExit;
	sol::function    _OnStateStartExiting;
	sol::function    _OnStateUpdate;

	std::vector<CStateRecord> _States;
	std::vector<std::pair<CStrID, sol::function>> _Interactions; // Interaction ID -> optional condition

public:

	CSmartObject(CStrID ID, CStrID DefaultState, std::string_view ScriptSource);

	virtual bool IsResourceValid() const override { return !_States.empty(); }

	// TODO: Add* logic to constructor? Or for runtime changes need also Remove*.
	bool         AddState(CStrID ID, CTimelineTask&& TimelineTask/*, state logic object ptr (optional)*/);
	bool         AddTransition(CStrID FromID, CStrID ToID, CTimelineTask&& TimelineTask);
	bool         AddInteraction(CStrID ID);
	bool         InitScript(sol::state& Lua);

	//???instead of it write OnState*(World, EntityID, StateID)?
	auto&        GetOnStateUpdateScript() { return _OnStateUpdate; }

	CStrID       GetDefaultState() const { return _DefaultState; }
	const auto&  GetInteractions() const { return _Interactions; }
};

}
