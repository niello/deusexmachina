#pragma once
#include <StdDEM.h>

// Base for all animation graph nodes

namespace DEM::Anim
{
using PAnimGraphNode = std::unique_ptr<class CAnimGraphNode>;
class CAnimationController;
class IPoseOutput;
struct CAnimationControllerInitContext;

enum class ESyncMethod //: U8
{
	None = 0,
	NormalizedTime,
	PhaseMatching
	// Markers?
};

struct CSyncContext
{
	ESyncMethod Method;
	float       NormalizedTime = 0.f;
	float       LocomotionPhase = std::numeric_limits<float>().lowest();
};

class CAnimGraphNode
{
protected:

	//??? bool Active ?

public:

	virtual ~CAnimGraphNode() {}

	//???default node must skip Update and return reference pose from eval?

	virtual void  Init(CAnimationControllerInitContext& Context) = 0;
	virtual void  Update(CAnimationController& Controller, float dt, CSyncContext* pSyncContext) = 0;
	virtual void  EvaluatePose(IPoseOutput& Output) = 0;

	virtual float GetAnimationLengthScaled() const { return 0.f; }
	virtual float GetLocomotionPhase() const { return std::numeric_limits<float>().lowest(); }
};

}
