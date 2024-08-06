#pragma once
#include <Scripting/Flow/FlowPlayer.h>

// A fork action that serves as a hub point for one or more incoming links that share further flow directions.
// It is also capable of selecting a random next link and can therefore serve as a random fork.

namespace DEM::Flow
{

class CHubAction : public Flow::IFlowAction
{
	FACTORY_CLASS_DECL;

public:

	static const CFlowLink* ChooseNext(const CFlowActionData& Proto, Game::CGameSession& Session, const CFlowVarStorage& Vars, Math::CWELL512& RNG);

	virtual void Update(Flow::CUpdateContext& Ctx) override;
};

}
