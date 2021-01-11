#pragma once
#include <Animation/Graph/AnimGraphNode.h>

// Animation graph node that plays a clip resource

namespace DEM::Anim
{
//using PClipPlayerNode = std::unique_ptr<class CClipPlayerNode>;

class CClipPlayerNode : public CAnimGraphNode
{
protected:

	// start offset, loop, playrate
	// clip itself, simple or composite (like montage but without logic)

public:

	//virtual bool BindOutput() - Prepare/Tune external output instead of binding and storing into the node?
	virtual void Update(float dt/*, params*/) override;
	virtual void EvaluatePose(/*IPoseOutput& but only if not bound!*/) override;
};

}
