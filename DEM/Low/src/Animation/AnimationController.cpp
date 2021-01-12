#include "AnimationController.h"
#include <Animation/Graph/AnimGraphNode.h>
#include <Animation/SkeletonInfo.h>

namespace DEM::Anim
{
CAnimationController::CAnimationController() = default;
CAnimationController::CAnimationController(CAnimationController&&) noexcept = default;
CAnimationController& CAnimationController::operator =(CAnimationController&&) noexcept = default;
CAnimationController::~CAnimationController() = default;

void CAnimationController::SetGraphRoot(PAnimGraphNode&& GraphRoot)
{
	if (_GraphRoot == GraphRoot) return;

	_GraphRoot = std::move(GraphRoot);
	_SkeletonInfo = nullptr;
}
//---------------------------------------------------------------------

void CAnimationController::Init(Resources::CResourceManager& ResMgr, std::map<CStrID, CStrID> AssetOverrides)
{
	if (_GraphRoot)
	{
		CAnimationControllerInitContext Context{ _SkeletonInfo, ResMgr, AssetOverrides };
		_GraphRoot->Init(Context);
	}

	// TODO: if !_SkeletonInfo here, can issue a warning - no leaf animation data is provided or some assets not resolved
}
//---------------------------------------------------------------------

void CAnimationController::Update(float dt)
{
	// update conditions etc

	if (_GraphRoot) _GraphRoot->Update(dt/*, params and other context*/);

	// synchronize times etc
}
//---------------------------------------------------------------------

void CAnimationController::EvaluatePose(IPoseOutput& Output)
{
	if (_GraphRoot) _GraphRoot->EvaluatePose(Output);
	//???else (if no _GraphRoot) leave as is or reset to refpose?
}
//---------------------------------------------------------------------

}
