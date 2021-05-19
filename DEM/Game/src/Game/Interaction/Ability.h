#pragma once
#include <Game/Interaction/Interaction.h>
#include <Game/Interaction/InteractionContext.h>
#include <Game/ECS/Components/ActionQueueComponent.h> // FIXME: for EActionStatus. Or define queue action type here too?!
#include <Game/ECS/Entity.h>
#include <Events/EventNative.h>
#include <Math/Matrix44.h>
#include <DetourNavMesh.h> // For dtPolyRef only

// Ability is an interaction performed by a actors in a game world.
// Ability is stateless, and ability instance stores the state.
// Inherit from CAbilityInstance if your ability requires additional state fields.

namespace DEM::Game
{
using PAbility = std::unique_ptr<class CAbility>;
using PAbilityInstance = std::unique_ptr<struct CAbilityInstance>;
class CZone;

enum class EAbilityExecutionStage : U8
{
	Movement = 0,
	Facing,
	Interaction
};

enum class EFacingMode : U8
{
	None = 0,  //???need? or use false return value?
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

//???TODO: to AbilityInstance.h?
class ExecuteAbility : public Events::CEventNative
{
	NATIVE_EVENT_DECL(ExecuteAbility, Events::CEventNative);

public:

	PAbilityInstance _AbilityInstance;

	explicit ExecuteAbility(PAbilityInstance&& AbilityInstance) : _AbilityInstance(std::move(AbilityInstance)) {}
};

//???TODO: to AbilityInstance.h?
//???where to handle cooldowns, costs etc?
//???how to treat targets ability-specific? E.g. what if between Execute & Start skill changed and fire ray count will change too?
//???in interaction context collect targets with some markers 'what it is' (roles)?
struct CAbilityInstance
{
	//???pass only certain fields from here and CInteractonContext to CInteraction::IsAvailable, to check
	//with the same code in iact mgr and during execution?

	const CAbility&           Ability; //???refcounted? or store as iact ID + SO ID? need to resolve each frame.
	HEntity                   Source; // E.g. item
	HEntity                   Actor;
	std::vector<CTargetInfo>  Targets;

	std::vector<const CZone*> InitialZones;
	std::vector<const CZone*> AvailableZones;
	UPTR                      CurrZoneIndex = INVALID_INDEX;

	matrix44                  TargetToWorld; // FIXME: not needed if CTargetInfo will store full SRT instead of Point!
	U32                       PrevTargetTfmVersion = 0;
	float                     ElapsedTime = -1.f;
	float                     PrevElapsedTime = 0.f; // Useful for dt calc and for detecting that we just passed some point in time
	EAbilityExecutionStage    Stage = EAbilityExecutionStage::Movement;
	dtPolyRef                 CheckedPoly = 0; // For path optimization

	CAbilityInstance(const CAbility& Ability_) : Ability(Ability_) {}
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
