#pragma once
#include <StdDEM.h>

// Base for all animation graph nodes

namespace DEM::Anim
{
using PAnimGraphNode = std::unique_ptr<class CAnimGraphNode>;
class CAnimationController;
class IPoseOutput;
struct CAnimationControllerInitContext;

class CAnimGraphNode
{
protected:

	//??? bool Active ?

public:

	virtual ~CAnimGraphNode() {}

	//???default node must skip Update and return reference pose from eval?

	virtual void Init(CAnimationControllerInitContext& Context) = 0;
	virtual void Update(CAnimationController& Controller, float dt) = 0;
	virtual void EvaluatePose(IPoseOutput& Output) = 0;
};

}
