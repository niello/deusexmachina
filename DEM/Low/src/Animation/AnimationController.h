#pragma once

// Animation controller plays an animation graph instance, feeding it with parameters.
// User specifies parameter values as an input and receives a pose as an output.

namespace DEM::Anim
{
//using PAnimationController = std::unique_ptr<class CAnimationController>;
class IPoseOutput;

class CAnimationController
{
protected:

	// parameters
	// shared conditions (allow nesting or not? if nested, must control cyclic dependencies and enforce calculation order)
	// NB: each condition, shared or not, must cache its value and recalculate only if used parameter values changed!

	// PAnimGraphNode _GraphRoot;

public:

	//???default node must skip Update and return reference pose from eval?

	void Init();
	//bool BindOutput() - Prepare/Tune external output instead of binding and storing into the node?
	void Update(float dt);
	void EvaluatePose(IPoseOutput& Output);

	// SetBool
	// SetInt
	// SetFloat
	// SetStrID
	// Get...?
};

}
