#include "NavAgentSettings.h"

namespace DEM::AI
{
RTTI_CLASS_IMPL(DEM::AI::CNavAgentSettings, Resources::CResourceObject);

CNavAgentSettings::CNavAgentSettings()
	//: _AgentRadius(AgentRadius)
	//, _AgentHeight(AgentHeight)
{
}
//---------------------------------------------------------------------

CNavAgentSettings::~CNavAgentSettings()
{
}
//---------------------------------------------------------------------

CTraversalController* CNavAgentSettings::FindController(unsigned char AreaType, dtPolyRef PolyRef) const
{
	//???could use flag instead of area, but flag must be accessed separately and area is already available.

	// if (AreaType == AREA_CONTROLLED)
	//   return FindPolyController(PolyRef);
	// else
	//   return FindAreaController(AreaType);

	//auto It = _Regions.find(ID);
	//return (It == _Regions.cend()) ? nullptr : &It->second;

	// FIXME: IMPLEMENT!!!
	return nullptr;
}
//---------------------------------------------------------------------

}
