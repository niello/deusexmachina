#include "BehaviourTreeExecuteAbility.h"
#include <Data/SerializeToParams.h>
#include <Core/Factory.h>

namespace DEM::AI
{
FACTORY_CLASS_IMPL(DEM::AI::CBehaviourTreeExecuteAbility, 'BTEA', CBehaviourTreeNodeBase);

void CBehaviourTreeExecuteAbility::Init(const Data::CParams* pParams)
{
	if (!pParams) return;

	// Ability ID
	// Target hardcoded ID / hardcoded list of IDs / BB key
}
//---------------------------------------------------------------------

EBTStatus CBehaviourTreeExecuteAbility::Activate(std::byte* pData) const
{
	// launch ability
	// fail if couldn't launch
	// receive HAction
	// if running, store it in pData
	// return status based on action status
	return EBTStatus::Failed;
}
//---------------------------------------------------------------------

void CBehaviourTreeExecuteAbility::Deactivate(std::byte* pData) const
{
	// clear HAction, cancel if still running
}
//---------------------------------------------------------------------

std::pair<EBTStatus, U16> CBehaviourTreeExecuteAbility::Update(U16 SelfIdx, float dt) const
{
	// return status based on action status
	return { EBTStatus::Failed, SelfIdx };
}
//---------------------------------------------------------------------

}
