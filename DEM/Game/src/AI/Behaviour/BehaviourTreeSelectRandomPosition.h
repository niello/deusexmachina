#pragma once
#include <AI/Behaviour/BehaviourTreeAsset.h>
#include <AI/Parameter.h>

// A BT action that selects a random point on the navmesh in a radius and sets it to the blackboard

namespace DEM::AI
{

class CBehaviourTreeSelectRandomPosition : public CBehaviourTreeNodeBase
{
	FACTORY_CLASS_DECL;

protected:

	CParameter<float> _Radius = 10.f;
	CStrID            _DestBBKey;

public:

	virtual void      Init(const Data::CParams* pParams) override;

	virtual EBTStatus Activate(std::byte* pData, const CBehaviourTreeContext& Ctx) const override;
};

}
