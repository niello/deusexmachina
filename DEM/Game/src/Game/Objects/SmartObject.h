#pragma once
#include <Resources/Resource.h>
#include <sol/sol.hpp>
#include <vector>

// Smart object asset describes a set of states, transitions between them,
// and interactions available over the object under different conditions

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

	CStrID        _DefaultState;
	std::string   _ScriptPath; //???CStrID of ScriptObject resource? Loader requires Lua.

	sol::function _OnStateEnter;
	sol::function _OnStateStartEntering;
	sol::function _OnStateExit;
	sol::function _OnStateStartExiting;
	sol::function _OnStateUpdate;

	std::vector<CStateRecord> _States; //!!!sort by ID for fast search!
	std::vector<std::pair<CStrID, sol::function>> _Interactions; // Interaction ID -> optional condition

public:

	CSmartObject(CStrID DefaultState, std::string_view ScriptPath);

	virtual bool IsResourceValid() const override { return !_States.empty(); }

	// TODO: to constructor? Not intended for runtime changes at least for now
	bool         AddState(CStrID ID, CTimelineTask&& TimelineTask/*, state logic object ptr (optional)*/);
	bool         AddTransition(CStrID FromID, CStrID ToID, CTimelineTask&& TimelineTask);
	bool         AddInteraction(CStrID ID);
	bool         InitScript(sol::state& Lua);

	const auto&  GetInteractions() const { return _Interactions; }
};

}
