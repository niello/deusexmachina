#include "BlendSpace1D.h"
#include <Animation/AnimationController.h>

namespace DEM::Anim
{

void CBlendSpace1D::Init(CAnimationControllerInitContext& Context)
{
	EParamType Type;
	if (!Context.Controller.FindParam(_ParamID, &Type, &_ParamIndex) || Type != EParamType::Float)
		_ParamIndex = INVALID_INDEX;

	if (!_Samples.empty())
	{
		for (auto& Sample : _Samples)
			Sample.Source->Init(Context);

		// Map blender to the total skeleton info (after initing child nodes!)
		// Allocate only two blender inputs
	}
}
//---------------------------------------------------------------------

void CBlendSpace1D::Update(CAnimationController& Controller, float dt)
{
	//!!!TODO: consider in logic!
	if (_Samples.size() < 2)
	{
		// special case, no need in input checking
	}

	const float Input = Controller.GetFloat(_ParamIndex);
	//???filter input to make transitions smoother?

	// calc animation speed to prevent foot sliding (lerp between sample speeds/durations by weight?)

	// calculate sources and weights based on input parameter
	// save sources and weights: float blend_factor = (m_value - low->value) / (high->value - low->value);
	// update active sources
	// maybe must do seek synchronization here, or record info and postpone after graph update
	//???sync only if clip player, or recurse down to players through any nodes?

	//!!!NB: sync may be abs time, normalized time (0 - 1) or by phase matching (calc from feet or manual)!
	//???!!!Phase matching with monotone value may be much better than named markers?
	//Curr time is obtained from the master track!
}
//---------------------------------------------------------------------

void CBlendSpace1D::EvaluatePose(IPoseOutput& Output)
{
	// use calculated sources and weights to blend final pose
	// request source poses from sources, must be already updated
	// use local outputs for blending, or passthrough for the first one, and then blend the second one inplace if needed
}
//---------------------------------------------------------------------

bool CBlendSpace1D::AddSample(PAnimGraphNode&& Source, float Value)
{
	if (!Source) return false;

	// find sorted pos
	// compare with neighbours
	//!!!Value must outstand at least tolerance from neighbour values!
	NOT_IMPLEMENTED;

	return false;
}
//---------------------------------------------------------------------

}
