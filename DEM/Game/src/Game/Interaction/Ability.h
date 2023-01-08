#pragma once
#include <Game/Interaction/Interaction.h>
#include <Game/ECS/Entity.h>
#include <Game/ECS/Components/ActionQueueComponent.h> // FIXME: for EActionStatus. Or define queue action type here too?!
#include <Math/Vector3.h>

// Ability is an interaction performed by actors in a game world

namespace DEM::Game
{
using PAbility = std::unique_ptr<class CAbility>;
using PAbilityInstance = Ptr<class CAbilityInstance>;
class CZone;
class CGameWorld;

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
protected:

	bool                  PushStandardExecuteAction(CGameWorld& World, HEntity Actor, const CInteractionContext& Context, bool Enqueue, bool PushChild) const;

	virtual PAbilityInstance CreateInstance(const CInteractionContext& Context) const;

public:

	virtual bool          GetZones(const CGameSession& Session, const CAbilityInstance& Instance, std::vector<const CZone*>& Out) const { return false; }
	virtual bool          GetFacingParams(const CGameSession& Session, const CAbilityInstance& Instance, CFacingParams& Out) const { return false; }
	virtual void          OnStart(CGameSession& Session, CAbilityInstance& Instance) const {}
	virtual EActionStatus OnUpdate(CGameSession& Session, CAbilityInstance& Instance) const { return EActionStatus::Succeeded; }
	virtual void          OnEnd(CGameSession& Session, CAbilityInstance& Instance, EActionStatus Status) const {}
};

}
