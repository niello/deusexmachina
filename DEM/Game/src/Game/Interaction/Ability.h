#pragma once
#include <Game/Interaction/Interaction.h>
#include <Game/ECS/Components/ActionQueueComponent.h> // FIXME: for EActionStatus. Or define queue action type here too?!
#include <Math/Vector3.h>

// Ability is an interaction performed by a actors in a game world

namespace DEM::Game
{
using PAbility = std::unique_ptr<class CAbility>;
class CAbilityInstance;
class CZone;

enum class EFacingMode : U8
{
	None = 0,  //???need? or return false from GetFacingParams?
	Direction,
	Point
	// TODO: SceneNode?
};

struct CFacingParams
{
	vector3     Dir;
	float       Tolerance = 0.f;
	EFacingMode Mode = EFacingMode::None;
};

class CAbility : public CInteraction
{
public:

	virtual bool          GetZones(const CAbilityInstance& Instance, std::vector<const CZone*>& Out) const = 0;
	virtual bool          GetFacingParams(const CAbilityInstance& Instance, CFacingParams& Out) const = 0;
	virtual void          OnStart(CAbilityInstance& Instance) const = 0;
	virtual EActionStatus OnUpdate(CAbilityInstance& Instance) const = 0;
	virtual void          OnEnd(CAbilityInstance& Instance, EActionStatus Status) const = 0;
};

}
