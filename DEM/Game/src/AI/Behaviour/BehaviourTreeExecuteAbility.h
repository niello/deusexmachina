#pragma once
#include <AI/Behaviour/BehaviourTreeAIActionBase.h>

// A leaf BT action that executes an ability by ID

namespace DEM::AI
{

class CBehaviourTreeExecuteAbility : public CBehaviourTreeAIActionBase
{
	FACTORY_CLASS_DECL;

protected:

	CStrID _AbilityID; //???allow BB key too?
	// Hardcoded targets or BB key, may be position, may need no target at all

public:

	virtual void                      Init(const Data::CParams* pParams) override;
	virtual size_t                    GetInstanceDataSize() const override;
	virtual size_t                    GetInstanceDataAlignment() const override;

	virtual EBTStatus                 Activate(std::byte* pData, const CBehaviourTreeContext& Ctx) const override;
	virtual void                      Deactivate(std::byte* pData, const CBehaviourTreeContext& Ctx) const override;
	virtual std::pair<EBTStatus, U16> Update(U16 SelfIdx, std::byte* pData, float dt, const CBehaviourTreeContext& Ctx) const override;
};

}
