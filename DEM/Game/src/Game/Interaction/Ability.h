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

	virtual bool          GetZones(const Game::CGameSession& Session, const CAbilityInstance& Instance, std::vector<const CZone*>& Out) const { return false; }
	virtual bool          GetFacingParams(const Game::CGameSession& Session, const CAbilityInstance& Instance, CFacingParams& Out) const { return false; }
	virtual void          OnStart(Game::CGameSession& Session, CAbilityInstance& Instance) const = 0;
	virtual EActionStatus OnUpdate(Game::CGameSession& Session, CAbilityInstance& Instance) const = 0;
	virtual void          OnEnd(Game::CGameSession& Session, CAbilityInstance& Instance, EActionStatus Status) const = 0;
};

}
