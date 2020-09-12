#include "SmartObject.h"

namespace DEM::Game
{
RTTI_CLASS_IMPL(DEM::Game::CSmartObject, Resources::CResourceObject);

bool CSmartObject::AddState(CStrID ID, CTimelineTask&& TimelineTask/*, state logic object ptr (optional)*/)
{
	// Check if state with the same ID exists
	auto It = std::lower_bound(_States.begin(), _States.end(), ID, [](const CStateRecord& State, CStrID StateID) { return State.ID < StateID; });
	if (It != _States.end() && (*It).ID == ID) return false;

	// Insert sorted by ID
	_States.insert(It, std::move(CStateRecord{ ID, std::move(TimelineTask) }));

	return true;
}
//---------------------------------------------------------------------

bool CSmartObject::AddTransition(CStrID FromID, CStrID ToID, CTimelineTask&& TimelineTask)
{
	NOT_IMPLEMENTED;
	return false;
}
//---------------------------------------------------------------------

bool CSmartObject::InitScript(sol::state_view& Lua)
{
	// load script from path into the named Lua object
	// cache state functions
	// cache interaction condition functions

	NOT_IMPLEMENTED;
	return false;
}
//---------------------------------------------------------------------

}
