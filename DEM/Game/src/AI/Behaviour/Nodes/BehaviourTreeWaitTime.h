#pragma once
#include <AI/Behaviour/BehaviourTreeAsset.h>
#include <AI/Parameter.h>

// A BT action that waits for a given time to pass

namespace DEM::AI
{

class CBehaviourTreeWaitTime : public CBehaviourTreeNodeBase
{
	FACTORY_CLASS_DECL;

protected:

	CParameter<float> _Time = 1.f; // seconds

public:

	virtual void                      Init(const Data::CParams* pParams) override;
	virtual size_t                    GetInstanceDataSize() const override;
	virtual size_t                    GetInstanceDataAlignment() const override;

	virtual EBTStatus                 Activate(std::byte* pData, const CBehaviourTreeContext& Ctx) const override;
	virtual void                      Deactivate(std::byte* pData, const CBehaviourTreeContext& Ctx) const override;
	virtual std::pair<EBTStatus, U16> Update(U16 SelfIdx, std::byte* pData, float dt, const CBehaviourTreeContext& Ctx) const override;
};

}
