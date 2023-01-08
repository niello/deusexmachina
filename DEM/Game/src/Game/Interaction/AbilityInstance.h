#pragma once
#include <Game/Interaction/InteractionContext.h>
#include <Game/ECS/Entity.h>
#include <Events/EventNative.h>
#include <Math/Matrix44.h>
#include <Data/Params.h>
#include <DetourNavMesh.h> // For dtPolyRef only

// Ability instance stores the state of a stateless ability and resembles one execution of it by an actor

namespace DEM::Game
{
using PAbilityInstance = Ptr<class CAbilityInstance>;
class CAbility;
class CZone;

enum class EAbilityExecutionStage : U8
{
	Movement = 0,
	Facing,
	Interaction
};

class ExecuteAbility : public ::Events::CEventNative
{
	NATIVE_EVENT_DECL(ExecuteAbility, ::Events::CEventNative);

public:

	PAbilityInstance _AbilityInstance;

	explicit ExecuteAbility(PAbilityInstance AbilityInstance) : _AbilityInstance(AbilityInstance) {}
};

//???where to handle cooldowns, costs etc?
//???how to treat targets ability-specific? E.g. what if between Execute & Start skill changed and
//fire ray count will change too? in interaction context store role of each target?
class CAbilityInstance : public Data::CRefCounted
{
public:

	//???pass only certain fields from here and CInteractonContext to CInteraction::IsAvailable, to check
	//with the same code in iact mgr and during execution?

	const CAbility&           Ability; //???FIXME: refcounted? or store as iact ID + SO ID? need to resolve each frame.
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

}
