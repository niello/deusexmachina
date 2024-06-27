#pragma once
#include <Core/Object.h>

// Reusable and stateless asset of a node based Flow script.
// Flow consists of actions linked with links which may be conditionally disabled.

namespace DEM::Flow
{

class CFlowAsset : public ::Core::CObject
{
	RTTI_CLASS_DECL(DEM::Flow::CFlowAsset, ::Core::CObject);

protected:

	// list of actions (map by ID? can deserialize individual actions and build map)
	// - unique ID (in a scope of this asset)
	// - type
	// - params
	// - list of links
	//   - target action ID
	//   - optional condition structure
	//     - type (some hardcoded + factory, or register all handlers by CStrID, including typical)
	//     - params
	//   - bool yield until next frame
	// default start action ID
	// default values for a variable storage

	//!!!values should support HEntity, but it is in DEMGame, maybe can use std::any? or some extensible type system? or move Flow to DEMGame?
	//!!!need variable storage default value deserialization for HEntity. Deserialization from HRD already exists, need to use properly.

public:
};

}
