#pragma once
#include <StdDEM.h>

// Base for all animation graph nodes

namespace DEM::Anim
{
using PAnimGraphNode = std::unique_ptr<class CAnimGraphNode>;
class CAnimationController;
class CPoseBuffer;
struct CAnimationInitContext;
struct CAnimationUpdateContext;

class CAnimGraphNode
{
public:

	virtual ~CAnimGraphNode() = default;

	//???default node must skip Update and return reference pose from eval?

	virtual void  Init(CAnimationInitContext& Context) = 0;
	virtual void  Update(CAnimationUpdateContext& Context, float dt) = 0;
	virtual void  EvaluatePose(CPoseBuffer& Output) = 0;

	virtual float GetAnimationLengthScaled() const { return 0.f; }
	virtual bool  IsActive() const { return true; }
};

}
