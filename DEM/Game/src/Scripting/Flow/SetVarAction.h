#pragma once
#include <Scripting/Flow/FlowPlayer.h>

// An action that sets a variable value in the flow player storage

namespace DEM::Flow
{

class CSetVarAction : public Flow::IFlowAction
{
	FACTORY_CLASS_DECL;

public:

	virtual void Update(Flow::CUpdateContext& Ctx) override;
};

}
