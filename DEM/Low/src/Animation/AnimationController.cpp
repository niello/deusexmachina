#include "AnimationController.h"
#include <Animation/Graph/AnimGraphNode.h>

namespace DEM::Anim
{

void CAnimationController::Init(Resources::CResourceManager& ResMgr, std::map<CStrID, CStrID> AssetOverrides)
{
	CAnimationControllerInitContext Context{ _SkeletonInfo, ResMgr, AssetOverrides };
	_GraphRoot->Init(Context);

	// TODO: if !_SkeletonInfo here, can issue a warning - no leaf animation data is provided or some assets not resolved
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
