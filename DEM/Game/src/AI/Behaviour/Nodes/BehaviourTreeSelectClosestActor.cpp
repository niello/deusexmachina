#include "BehaviourTreeSelectClosestActor.h"
#include <AI/AIStateComponent.h>
#include <Game/GameSession.h>
#include <Game/ECS/GameWorld.h>
#include <Scene/SceneComponent.h>
#include <Core/Factory.h>

namespace DEM::AI
{
FACTORY_CLASS_IMPL(DEM::AI::CBehaviourTreeSelectClosestActor, 'BTCA', CBehaviourTreeNodeBase);

//!!!TODO: move target filtering code to a separate target selection module!

/* // Need mutable context to remember calculated actor position and maybe to return calculated distance to fact
struct CCloserThanFilter
{
	float MaxSqDistance = std::numeric_limits<float>::max();

	CCloserThanFilter(float MaxDistance) : MaxSqDistance(MaxDistance * MaxDistance) {}

	bool operator()(const CSensedStimulus& Fact, const CBehaviourTreeContext& Ctx) const
	{
	//???store distance in a fact when the fact is added/last updated?! may become stale, then will need to be recalculated and will be stored back.
	//can use actor tfm version. Does worth an effort? Accessing tfm version requires checking actor scene node. But can cache it in Ctx.
		const float SqDist = rtm::vector_length_squared3(rtm::vector_sub(ActorPos, Fact.Position));
		return SqDist <= MaxSqDistance;
	}
};

struct CIsActorFilter
{
	bool operator()(const CSensedStimulus& Fact, const CBehaviourTreeContext& Ctx) const
	{
		// FIXME: provide world in a Ctx!
		auto* pWorld = Ctx.Session.FindFeature<Game::CGameWorld>();
		return !!pWorld->FindComponent<const CAIStateComponent>(Fact.SourceID);
	}
};

template <typename... TFilters>
auto MakeCombinedPredicateImpl(std::true_type, TFilters... Filters)
{
	return [=](const CSensedStimulus& Fact, const CBehaviourTreeContext& Ctx) -> bool
	{
		return (Filters(Fact, Ctx) && ...);
	};
}

[[deprecated("AI FILTER WARNING: Filters are not in optimal cost order (cheapest to most expensive). Performance may be impacted.")]]
template <typename... TFilters>
auto MakeCombinedPredicateImpl(std::false_type, TFilters... Filters)
{
	return MakeCombinedPredicateImpl(std::true_type{}, Filters...);
}

constexpr bool IsListSorted(std::initializer_list<int> Values)
{
	int PrevValue = 0;
	for (int Value : Values)
	{
		if (Value < PrevValue) return false;
		PrevValue = Value;
	}
	return true;
}

template <typename... TFilters>
auto MakeCombinedPredicate(TFilters... Filters)
{
	constexpr bool IsSortedByCost = IsListSorted({ FilterCost<TFilters>::value... });
	return MakeCombinedPredicateImpl(std::bool_constant<IsSortedByCost>{}, Filters...);
}

// Use like:
//auto my_predicate = MakeCombinedPredicate(
//	CFlagsFilter{EStimulusType::Danger | EStimulusType::Threat},
//	CCloserThanFilter{100.f},
//  ...
//);

template <typename FPredicate, typename FCallback>
void FilterFacts(const std::vector<CSensedStimulus>& Facts, const CBehaviourTreeContext& Ctx, FPredicate Predicate, FCallback Callback)
{
	for (const auto& Fact : Facts)
		if (Predicate(Fact, Ctx))
			Callback(Fact);
}
//---------------------------------------------------------------------

// then scoring and finding 1 max or N max (std::partial_sort)
*/

//!!!DBG TMP! The whole CBehaviourTreeSelectClosestActor is mostly a demonstration of target selection architecture.
// This selector must be moved to utility functions or maybe it is not useful and must be removed in favour of more practical ones.
static Game::HEntity FindClosestActor(const CBehaviourTreeContext& Ctx)
{
	auto* pWorld = Ctx.Session.FindFeature<Game::CGameWorld>();
	if (!pWorld) return {};

	auto* pActorScene = pWorld->FindComponent<const Game::CSceneComponent>(Ctx.ActorID);
	if (!pActorScene) return {};

	const rtm::vector4f ActorPos = pActorScene->RootNode->GetWorldPosition();

	Game::HEntity Result;
	float MinSqDist = std::numeric_limits<float>::max();
	for (const auto& Fact : Ctx.pBrain->Facts)
	{
		if (!Fact.SourceID) continue;

		const float SqDist = rtm::vector_length_squared3(rtm::vector_sub(ActorPos, Fact.Position));
		if (MinSqDist <= SqDist) continue;

		// Filter by AI actors only
		if (!pWorld->FindComponent<const CAIStateComponent>(Fact.SourceID)) continue;

		MinSqDist = SqDist;
		Result = Fact.SourceID;
	}

	return Result;
}
//---------------------------------------------------------------------

void CBehaviourTreeSelectClosestActor::Init(const Data::CParams* pParams)
{
	if (!pParams) return;

	pParams->TryGet(_BBKey, CStrID("BBKey"));
	pParams->TryGet(_Period, CStrID("Period"));
}
//---------------------------------------------------------------------

size_t CBehaviourTreeSelectClosestActor::GetInstanceDataSize() const
{
	return sizeof(CInstanceData);
}
//---------------------------------------------------------------------

size_t CBehaviourTreeSelectClosestActor::GetInstanceDataAlignment() const
{
	return alignof(CInstanceData);
}
//---------------------------------------------------------------------

void CBehaviourTreeSelectClosestActor::DoSelection(const CBehaviourTreeContext& Ctx, CInstanceData& Data) const
{
	const auto FoundID = FindClosestActor(Ctx);

	//???TODO: in OnTreeStarted cache BB key HVar?
	Ctx.pBrain->Blackboard.Set(_BBKey, static_cast<int>(FoundID.Raw));

	Data.TimeToNextUpdate = _Period;
	//Data.FactsVersion = Ctx.pBrain->FactsVersion;
}
//---------------------------------------------------------------------

EBTStatus CBehaviourTreeSelectClosestActor::Activate(std::byte* pData, const CBehaviourTreeContext& Ctx) const
{
	// Before everything else because Deactivate always calls a destructor
	auto& Data = *new(pData) CInstanceData();

	if (_BBKey) DoSelection(Ctx, Data);

	// We are merely a service that provides data to the underlying subtree, always proceed to it
	return EBTStatus::Running;
}
//---------------------------------------------------------------------

void CBehaviourTreeSelectClosestActor::Deactivate(std::byte* pData, const CBehaviourTreeContext& Ctx) const
{
	//!!!TODO: need to check if should clear BB record!
	if (_BBKey) Ctx.pBrain->Blackboard.Set(_BBKey, static_cast<int>(Game::HEntity{}.Raw));

	std::destroy_at(reinterpret_cast<CInstanceData*>(pData));
}
//---------------------------------------------------------------------

std::pair<EBTStatus, U16> CBehaviourTreeSelectClosestActor::Update(U16 SelfIdx, std::byte* pData, float dt, const CBehaviourTreeContext& Ctx) const
{
	auto& Data = *reinterpret_cast<CInstanceData*>(pData);

	// TODO: also check Data.FactsVersion!
	if (Data.TimeToNextUpdate > dt)
	{
		Data.TimeToNextUpdate -= dt;
		return { EBTStatus::Running, SelfIdx };
	}

	DoSelection(Ctx, Data);

	return { EBTStatus::Running, SelfIdx };
}
//---------------------------------------------------------------------

}
