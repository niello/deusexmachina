#pragma once

// Base for all animation graph nodes

namespace DEM::Anim
{
//using PAnimGraphNode = std::unique_ptr<class CAnimGraphNode>;

class CAnimGraphNode
{
protected:

	//

public:

	//???default node must skip Update and return reference pose from eval?

	//virtual bool BindOutput() - Prepare/Tune external output instead of binding and storing into the node?
	virtual void Update(float dt/*, params*/) = 0;
	virtual void EvaluatePose(/*IPoseOutput& but only if not bound!*/) = 0;
};

}
