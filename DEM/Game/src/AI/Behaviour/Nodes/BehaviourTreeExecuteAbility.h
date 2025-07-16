#pragma once
#include <AI/Behaviour/BehaviourTreeAsset.h>
#include <AI/Parameter.h>

// A leaf BT action that executes an ability by ID

namespace DEM::AI
{

class CBehaviourTreeExecuteAbility : public CBehaviourTreeNodeBase
{
	FACTORY_CLASS_DECL;

protected:

	CParameter<CStrID>                         _AbilityID;
	CParameterEx<Game::HEntity, rtm::vector4f> _Target; // TODO: support an array of targets? for multi-target abilities

public:

	virtual void                      Init(const Data::CParams* pParams) override;
	virtual size_t                    GetInstanceDataSize() const override;
	virtual size_t                    GetInstanceDataAlignment() const override;

	virtual EBTStatus                 Activate(std::byte* pData, const CBehaviourTreeContext& Ctx) const override;
	virtual void                      Deactivate(std::byte* pData, const CBehaviourTreeContext& Ctx) const override;
	virtual std::pair<EBTStatus, U16> Update(U16 SelfIdx, std::byte* pData, float dt, const CBehaviourTreeContext& Ctx) const override;
};

}
