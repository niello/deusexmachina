#include "BlendSpace1D.h"

namespace DEM::Anim
{

void CBlendSpace1D::Init(/*some params?*/)
{
}
//---------------------------------------------------------------------

void CBlendSpace1D::Update(float dt/*, params*/)
{
	// calculate sources and weights based on input parameter
	// save sources and weights
	// maybe must do seek synchronization here
}
//---------------------------------------------------------------------

void CBlendSpace1D::EvaluatePose(IPoseOutput& Output)
{
	// use calculated sources and weights to blend final pose
	// request source poses from sources, must be already updated
	// use local outputs for blending, or passthrough for the first one, and then blend the second one inplace if needed
}
//---------------------------------------------------------------------

}
