#pragma once
#include <AI/Behaviour/BehaviourTreeAsset.h>

// A leaf BT action that executes an ability by ID

namespace DEM::AI
{

class CBehaviourTreeExecuteAbility : public CBehaviourTreeNodeBase
{
	FACTORY_CLASS_DECL;

protected:

	// Abitily ID
	// Hardcoded targets or BB key, may be position, may need no target at all

public:

	virtual void                      Init(const Data::CParams* pParams) override;

	virtual EBTStatus                 Activate(std::byte* pData) const override;
	virtual void                      Deactivate(std::byte* pData) const override;
	virtual std::pair<EBTStatus, U16> Update(U16 SelfIdx, float dt) const override;
};

}
