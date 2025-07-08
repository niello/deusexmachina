#pragma once
#include <AI/Behaviour/Nodes/BehaviourTreeAIActionBase.h>
#include <AI/Parameter.h>

// A leaf BT action that orders an agent to move to the desired position or entity

namespace DEM::AI
{

class CBehaviourTreeMove : public CBehaviourTreeAIActionBase
{
	FACTORY_CLASS_DECL;

protected:

	CParameterEx<rtm::vector4f, Game::HEntity> _Target;
	bool _IgnoreNavigation = false; // Move directly to the destination, ignore navmesh and path controllers
	bool _Follow = false;

	//???add tolerance as a parameter?

	std::optional<rtm::vector4f> GetPosition(const CBehaviourTreeContext& Ctx) const;

public:

	virtual void                      Init(const Data::CParams* pParams) override;
	virtual size_t                    GetInstanceDataSize() const override;
	virtual size_t                    GetInstanceDataAlignment() const override;

	virtual EBTStatus                 Activate(std::byte* pData, const CBehaviourTreeContext& Ctx) const override;
	virtual void                      Deactivate(std::byte* pData, const CBehaviourTreeContext& Ctx) const override;
	virtual std::pair<EBTStatus, U16> Update(U16 SelfIdx, std::byte* pData, float dt, const CBehaviourTreeContext& Ctx) const override;
};

}
