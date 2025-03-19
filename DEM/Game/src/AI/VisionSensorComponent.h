#pragma once
#include <Scene/SceneNode.h>
#include <Data/Metadata.h>
#include <Math/Math.h>

// A vision sensor component is able to detect vision stimuli, i.e. visible objects

namespace DEM::AI
{

struct CVisionSensorComponent
{
	// Calculated from serializable properties
	Scene::PSceneNode Node;
	float             CosHalfPerfectFOV = 1.f;
	float             CosHalfMaxFOV = 1.f;

	float             PerfectRadius = 10.f;
	float             MaxRadius = 30.f;
	float             PerfectFOV = n_deg2rad(60.f);
	float             MaxFOV = n_deg2rad(120.f);
	std::string       NodePath;
};

}

namespace DEM::Meta
{

template<> constexpr auto RegisterClassName<AI::CVisionSensorComponent>() { return "DEM::AI::CVisionSensorComponent"; }
template<> constexpr auto RegisterMembers<AI::CVisionSensorComponent>()
{
	return std::make_tuple
	(
		DEM_META_MEMBER_FIELD(AI::CVisionSensorComponent, PerfectRadius),
		DEM_META_MEMBER_FIELD(AI::CVisionSensorComponent, MaxRadius),
		DEM_META_MEMBER_FIELD(AI::CVisionSensorComponent, PerfectFOV),
		DEM_META_MEMBER_FIELD(AI::CVisionSensorComponent, MaxFOV),
		DEM_META_MEMBER_FIELD(AI::CVisionSensorComponent, NodePath)
	);
}

}
