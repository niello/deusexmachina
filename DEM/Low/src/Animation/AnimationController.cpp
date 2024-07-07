#include "AnimationController.h"
#include <Animation/Graph/AnimGraphNode.h>
#include <Animation/SkeletonInfo.h>
#include <Animation/Skeleton.h>

namespace DEM::Anim
{
CAnimationController::CAnimationController() = default;
CAnimationController::CAnimationController(CAnimationController&&) noexcept = default;
CAnimationController& CAnimationController::operator =(CAnimationController&&) noexcept = default;
CAnimationController::~CAnimationController() = default;

void CAnimationController::Init(PAnimGraphNode&& GraphRoot, Resources::CResourceManager& ResMgr,
	CStrID LeftFootID, CStrID RightFootID,
	std::map<CStrID, float>&& Floats,
	std::map<CStrID, int>&& Ints,
	std::map<CStrID, bool>&& Bools,
	std::map<CStrID, CStrID>&& Strings,
	const std::map<CStrID, CStrID>& AssetOverrides)
{
	_Params.clear();

	if (!Floats.empty())
	{
		_Params.reserve<float>(Floats.size());
		for (const auto [ID, DefaultValue] : Floats)
			_Params.Set(ID, DefaultValue);
	}

	if (!Ints.empty())
	{
		_Params.reserve<int>(Ints.size());
		for (const auto [ID, DefaultValue] : Ints)
			_Params.Set(ID, DefaultValue);
	}

	if (!Bools.empty())
	{
		_Params.reserve<bool>(Bools.size());
		for (const auto [ID, DefaultValue] : Bools)
			_Params.Set(ID, DefaultValue);
	}

	if (!Strings.empty())
	{
		_Params.reserve<CStrID>(Strings.size());
		for (const auto [ID, DefaultValue] : Strings)
			_Params.Set(ID, DefaultValue);
	}

	_GraphRoot = std::move(GraphRoot);
	_SkeletonInfo = nullptr;
	_UpdateCounter = 0;

	if (_GraphRoot)
	{
		CAnimationInitContext Context{ *this, _SkeletonInfo, ResMgr, AssetOverrides };
		_GraphRoot->Init(Context);
	}

	_PoseIndex = 2;
	if (_SkeletonInfo)
	{
		_LastPoses[0].SetSize(_SkeletonInfo->GetNodeCount());
		_LastPoses[1].SetSize(_SkeletonInfo->GetNodeCount());

		_LeftFootBoneIndex = _SkeletonInfo->FindNodePort(LeftFootID);
		_RightFootBoneIndex = _SkeletonInfo->FindNodePort(RightFootID);
	}
	else
	{
		// TODO: can issue a warning - no leaf animation data is provided or some assets are not resolved
		_LeftFootBoneIndex = INVALID_BONE_INDEX;
		_RightFootBoneIndex = INVALID_BONE_INDEX;
	}
}
//---------------------------------------------------------------------

void CAnimationController::Update(const CSkeleton& Target, float dt, Events::IEventOutput* pEventOutput)
{
	ZoneScoped;

	// update conditions etc

	// TODO:
	// - pose modifiers = skeletal controls (like lookat), object space (like rigid body or what?)
	// - IK

	if (_GraphRoot)
	{
		++_UpdateCounter;
		CAnimationUpdateContext Context{ *this, Target, pEventOutput };
		_GraphRoot->Update(Context, dt);

		// TODO: synchronize times by sync group?
	}

	_InertializationDt += dt; // Don't write to _InertializationElapsedTime, because dt may be discarded by teleportation
}
//---------------------------------------------------------------------

void CAnimationController::EvaluatePose(CSkeleton& Target)
{
	ZoneScoped;

	Target.ToPoseBuffer(_CurrPose); // TODO: update only if changed externally?

	if (_PoseIndex > 1)
	{
		// Init both poses from current. Should be used at the first frame and when teleported.
		_PoseIndex = 0;
		_LastPoses[0] = _CurrPose;
		_LastPoses[1] = _CurrPose;
		_LastPoseDt = 0.f;
	}
	else
	{
		// Swap current and previous pose buffers
		_PoseIndex ^= 1;
		_LastPoses[_PoseIndex] = _CurrPose;
		_LastPoseDt = _InertializationDt;
	}

	if (_GraphRoot) _GraphRoot->EvaluatePose(_CurrPose);
	//???else (if no _GraphRoot) leave as is or reset to refpose?

	ProcessInertialization();

	//!!!DBG TMP!
	//pseudo root motion processing
	//???diff from ref pose? can also be from the first frame of the animation, but that complicates things
	//???precalculate something in tools to simplify processing here? is possible?
	//Output.SetTranslation(0, vector3::Zero);

	Target.FromPoseBuffer(_CurrPose);
}
//---------------------------------------------------------------------

float CAnimationController::GetLocomotionPhaseFromPose(const CSkeleton& Skeleton) const
{
	if (_LeftFootBoneIndex == INVALID_BONE_INDEX || _RightFootBoneIndex == INVALID_BONE_INDEX) return -1.f;

	const auto* pRootNode = Skeleton.GetNode(0);
	const auto* pLeftFootNode = Skeleton.GetNode(_LeftFootBoneIndex);
	const auto* pRightFootNode = Skeleton.GetNode(_RightFootBoneIndex);
	if (!pRootNode || !pLeftFootNode || !pRightFootNode) return -1.f;

	// FIXME: isn't forward -Z?
	const rtm::vector4f ForwardDir = rtm::vector_normalize3(pRootNode->GetWorldMatrix().z_axis);
	const rtm::vector4f SideDir = rtm::vector_normalize3(pRootNode->GetWorldMatrix().x_axis);

	// Project foot offset onto the locomotion plane (fwd, up) and normalize it to get phase direction
	const rtm::vector4f FootOffset = rtm::vector_sub(pLeftFootNode->GetWorldMatrix().w_axis, pRightFootNode->GetWorldMatrix().w_axis);
	const rtm::vector4f ProjectedFootOffset = rtm::vector_sub(FootOffset, rtm::vector_mul(SideDir, (rtm::scalarf)rtm::vector_dot3(FootOffset, SideDir)));
	const rtm::vector4f PhaseDir = rtm::vector_normalize3(ProjectedFootOffset);

	const float CosA = rtm::vector_dot3(PhaseDir, ForwardDir);
	const float SinA = rtm::vector_dot3(rtm::vector_cross3(PhaseDir, ForwardDir), SideDir);

	const float Angle = std::copysignf(std::acosf(CosA) * 180.f / PI, SinA); // Could also use Angle = RadToDeg(std::atan2f(SinA, CosA));

	return 180.f - Angle; // map 180 -> -180 to 0 -> 360
}
//---------------------------------------------------------------------

// Returns a duration of one playback iteration if applicable
float CAnimationController::GetExpectedAnimationLength() const
{
	return _GraphRoot ? _GraphRoot->GetAnimationLengthScaled() : 0.f;
}
//---------------------------------------------------------------------

void CAnimationController::RequestInertialization(float Duration)
{
	if (_InertializationRequest < 0.f || _InertializationRequest > Duration)
		_InertializationRequest = Duration;
}
//---------------------------------------------------------------------

// TODO: detect teleportation, reset pending request and _InertializationDt to mitigate abrupt velocity changes
// That must zero out _LastPoseDt, when it is not a last dt yet!
void CAnimationController::ProcessInertialization()
{
	if (_PoseIndex > 1) return;

	// Process new request
	const bool RequestPending = (_InertializationRequest > 0.f);
	if (RequestPending)
	{
		float NewDuration = _InertializationRequest;
		_InertializationRequest = -1.f;

		// When interrupt active inertialization, must reduce duration to avoid degenerate pose (see UE4)
		if (_InertializationDuration > 0.f)
		{
			// TODO: learn why we check prevoius deficit to apply current
			const bool HasDeficit = (_InertializationDeficit > 0.f);
			_InertializationDeficit = _InertializationDuration - _InertializationElapsedTime;
			if (HasDeficit) NewDuration = std::max(0.f, NewDuration - _InertializationDeficit);
		}

		_InertializationDuration = NewDuration;
		_InertializationElapsedTime = 0.0f;
	}

	// Process active inertialization
	if (_InertializationDuration > 0.f)
	{
		if (RequestPending)
			_InertializationPoseDiff.Init(_CurrPose, _LastPoses[_PoseIndex], _LastPoses[_PoseIndex ^ 1], _LastPoseDt, _InertializationDuration);

		_InertializationElapsedTime += _InertializationDt;
		if (_InertializationElapsedTime >= _InertializationDuration)
		{
			_InertializationElapsedTime = 0.0f;
			_InertializationDuration = 0.0f;
			_InertializationDeficit = 0.0f;
		}
		else
		{
			_InertializationDeficit = std::max(0.f, _InertializationDeficit - _InertializationDt);
			_InertializationPoseDiff.ApplyTo(_CurrPose, _InertializationElapsedTime);
		}
	}

	_InertializationDt = 0.f;
}
//---------------------------------------------------------------------

}
