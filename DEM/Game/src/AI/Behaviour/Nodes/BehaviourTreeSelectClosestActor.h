#pragma once
#include <AI/Behaviour/BehaviourTreeAsset.h>

// A BT decorator that monitors the closes AI actor and writes its ID to the blackboard

namespace DEM::AI
{

class CBehaviourTreeSelectClosestActor : public CBehaviourTreeNodeBase
{
	FACTORY_CLASS_DECL;

protected:

	struct CInstanceData
	{
		U32   FactsVersion = 0;
		float TimeToNextUpdate = 0.f;
	};

	CStrID _DestBBKey;
	float  _Period = 1.5f; // seconds between updates

	void DoSelection(const CBehaviourTreeContext& Ctx, CInstanceData& Data) const;

public:

	virtual void                      Init(const Data::CParams* pParams) override;
	virtual size_t                    GetInstanceDataSize() const override;
	virtual size_t                    GetInstanceDataAlignment() const override;

	virtual EBTStatus                 Activate(std::byte* pData, const CBehaviourTreeContext& Ctx) const override;
	virtual void                      Deactivate(std::byte* pData, const CBehaviourTreeContext& Ctx) const override;
	virtual std::pair<EBTStatus, U16> Update(U16 SelfIdx, std::byte* pData, float dt, const CBehaviourTreeContext& Ctx) const override;
};

}
