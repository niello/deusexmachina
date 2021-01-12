#include "AnimationController.h"

namespace DEM::Anim
{

void CAnimationController::Init()
{
	//!!!pass asset resolve and override data!
	// _GraphRoot->Init();
}
//---------------------------------------------------------------------

void CAnimationController::Update(float dt)
{
	// update conditions etc

	// _GraphRoot->Update(dt, params and other context);

	// synchronize times etc
}
//---------------------------------------------------------------------

void CAnimationController::EvaluatePose(IPoseOutput& Output)
{
	// _GraphRoot->EvaluatePose(Output);
	// if no root, leave as is or reset to refpose?
}
//---------------------------------------------------------------------

}
