#include "BehaviourTreeWaitTime.h"
#include <AI/AIStateComponent.h>
#include <Game/GameSession.h>
#include <Game/ECS/GameWorld.h>
#include <Scene/SceneComponent.h>
#include <Core/Factory.h>

namespace DEM::AI
{
FACTORY_CLASS_IMPL(DEM::AI::CBehaviourTreeWaitTime, 'BTWT', CBehaviourTreeNodeBase);

void CBehaviourTreeWaitTime::Init(const Data::CParams* pParams)
{
	if (!pParams) return;

	ParameterFromData(_Time, *pParams, CStrID("Time"));
}
//---------------------------------------------------------------------

size_t CBehaviourTreeWaitTime::GetInstanceDataSize() const
{
	return sizeof(float);
}
//---------------------------------------------------------------------

size_t CBehaviourTreeWaitTime::GetInstanceDataAlignment() const
{
	return alignof(float);
}
//---------------------------------------------------------------------

EBTStatus CBehaviourTreeWaitTime::Activate(std::byte* pData, const CBehaviourTreeContext& Ctx) const
{
	const float RemainingTime = _Time.Get(Ctx.pBrain->Blackboard);
	n_assert_dbg(RemainingTime > 0.f);

	// Before everything else because Deactivate always calls a destructor
	new(pData) float(RemainingTime);

	return (RemainingTime > 0.f) ? EBTStatus::Running : EBTStatus::Succeeded;
}
//---------------------------------------------------------------------

void CBehaviourTreeWaitTime::Deactivate(std::byte* pData, const CBehaviourTreeContext& Ctx) const
{
	std::destroy_at(reinterpret_cast<float*>(pData));
}
//---------------------------------------------------------------------

std::pair<EBTStatus, U16> CBehaviourTreeWaitTime::Update(U16 SelfIdx, std::byte* pData, float dt, const CBehaviourTreeContext& Ctx) const
{
	float& RemainingTime = *reinterpret_cast<float*>(pData);

	if (RemainingTime > dt)
	{
		RemainingTime -= dt;
		return { EBTStatus::Running, SelfIdx };
	}

	return { EBTStatus::Succeeded, SelfIdx };
}
//---------------------------------------------------------------------

}
