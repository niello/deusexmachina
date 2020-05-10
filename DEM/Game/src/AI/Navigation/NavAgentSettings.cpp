#include "NavAgentSettings.h"
#include <AI/Navigation/TraversalController.h>

namespace DEM::AI
{
RTTI_CLASS_IMPL(DEM::AI::CNavAgentSettings, Resources::CResourceObject);

CNavAgentSettings::CNavAgentSettings(std::map<U8, float>&& Costs, std::vector<DEM::AI::PTraversalController>&& Controllers)
	: _Controllers(std::move(Controllers))
{
	// NB: it is not recommended in Detour docs to use costs less than 1.0
	for (auto [Area, Cost] : Costs)
		if (Cost > 1.f)
			_Filter.setAreaCost(Area, Cost);
}
//---------------------------------------------------------------------

CNavAgentSettings::~CNavAgentSettings()
{
}
//---------------------------------------------------------------------

CTraversalController* CNavAgentSettings::FindController(unsigned char AreaType, dtPolyRef PolyRef) const
{
	if (AreaType > 63 || AreaType >= _Controllers.size()) return nullptr;

	// TODO: per-poly controllers
	//???could use flag instead of area, but flag must be accessed separately and area is already available.
	// if (AreaType == AREA_CONTROLLED)
	//   return FindPolyController(PolyRef);
	// else

	return _Controllers[AreaType];
}
//---------------------------------------------------------------------

}
