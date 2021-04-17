#pragma once
#include <Math/Vector3.h>
#include <Game/ECS/Components/ActionQueueComponent.h> // FIXME: for EActionStatus. Or define queue action type here too?!

// Ability is a stateless piece of actor's logic. Most of the time this logic
// resembles some interaction with a target. An actor itself can be its own target.
// Ability instance stores the state needed to track interaction progress per actor per time.
// You should inherit from CAbilityInstance if you need special state for your CAbility.

namespace DEM::Game
{
using PAbility = std::unique_ptr<class CAbility>;
using PAbilityInstance = std::unique_ptr<class CAbilityInstance>;

enum class EFacingMode : U8
{
	None = 0,  //???need? or use false return value?
	Direction,
	Point
	// TODO: SceneNode?
};

struct CFacingParams
{
	EFacingMode Mode;
	vector3     Dir;
	float       Tolerance = 0.f;
};

class CAbilityInstance
{
protected:

	const CAbility& _Ability; //???!!!can guarantee lifetime?! or use refcounted?

public:

	CAbilityInstance(const CAbility& Ability) : _Ability(Ability) {}

	float           ElapsedTime = 0.f;
	float           PrevElapsedTime = 0.f; // Useful for dt calc and for detecting that we just passed some point in time
};

class CAbility
{
public:

	virtual ~CAbility() = default;

	virtual PAbilityInstance CreateInstance() const { return PAbilityInstance(n_new(CAbilityInstance(*this))); }

	virtual vector3          GetInteractionPoint() const = 0;
	virtual bool             GetFacingParams(CFacingParams& Out) const = 0;
	virtual void             OnStart() const = 0; // ActorEntity, TargetEntity
	virtual EActionStatus    OnUpdate() const = 0; // ActorEntity, TargetEntity, AbilityInstance
	virtual void             OnEnd() const = 0; // ActorEntity, TargetEntity, Status
};

}
