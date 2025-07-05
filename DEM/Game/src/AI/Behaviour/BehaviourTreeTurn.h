#pragma once
#include <AI/Behaviour/BehaviourTreeAIActionBase.h>
#include <AI/Parameter.h>

// A leaf BT action that orders an agent to turn to the desired direction or entity

namespace DEM::AI
{

class CBehaviourTreeTurn : public CBehaviourTreeAIActionBase
{
	FACTORY_CLASS_DECL;

protected:

	CParameterEx<rtm::vector4f, Game::HEntity, float> _Target; // lookat point / entity / angle (-180 .. 180)
	bool _Follow = false;

	//???add tolerance as a parameter?

	std::optional<rtm::vector4f> GetDirection(const CBehaviourTreeContext& Ctx) const;

public:

	virtual void                      Init(const Data::CParams* pParams) override;
	virtual size_t                    GetInstanceDataSize() const override;
	virtual size_t                    GetInstanceDataAlignment() const override;

	virtual EBTStatus                 Activate(std::byte* pData, const CBehaviourTreeContext& Ctx) const override;
	virtual void                      Deactivate(std::byte* pData, const CBehaviourTreeContext& Ctx) const override;
	virtual std::pair<EBTStatus, U16> Update(U16 SelfIdx, std::byte* pData, float dt, const CBehaviourTreeContext& Ctx) const override;
};

}
