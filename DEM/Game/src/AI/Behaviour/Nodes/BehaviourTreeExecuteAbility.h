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

	CParameter<CStrID>        _AbilityID;
	//CParameter<Game::HEntity> _Target; //!!!TODO: hardcoded target or multiple, or BB key, may be position, may need no target at all

public:

	virtual void                      Init(const Data::CParams* pParams) override;
	virtual size_t                    GetInstanceDataSize() const override;
	virtual size_t                    GetInstanceDataAlignment() const override;

	virtual EBTStatus                 Activate(std::byte* pData, const CBehaviourTreeContext& Ctx) const override;
	virtual void                      Deactivate(std::byte* pData, const CBehaviourTreeContext& Ctx) const override;
	virtual std::pair<EBTStatus, U16> Update(U16 SelfIdx, std::byte* pData, float dt, const CBehaviourTreeContext& Ctx) const override;
};

}
