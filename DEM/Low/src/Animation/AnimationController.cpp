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
	std::map<CStrID, CStrID>&& StrIDs,
	const std::map<CStrID, CStrID>& AssetOverrides)
{
	_Params.clear();
	_FloatValues.reset();
	_IntValues.reset();
	_BoolValues.reset();
	_StrIDValues.reset();

	if (!Floats.empty())
	{
		UPTR CurrIdx = 0;
		_FloatValues.reset(new float[Floats.size()]);
		for (const auto [ID, DefaultValue] : Floats)
		{
			_Params.emplace(ID, std::pair{ EParamType::Float, CurrIdx });
			_FloatValues[CurrIdx++] = DefaultValue;

			// Duplicate IDs aren't allowed
			Ints.erase(ID);
			Bools.erase(ID);
			StrIDs.erase(ID);
		}
	}

	if (!Ints.empty())
	{
		UPTR CurrIdx = 0;
		_IntValues.reset(new int[Ints.size()]);
		for (const auto [ID, DefaultValue] : Ints)
		{
			_Params.emplace(ID, std::pair{ EParamType::Int, CurrIdx });
			_IntValues[CurrIdx++] = DefaultValue;

			// Duplicate IDs aren't allowed
			Bools.erase(ID);
			StrIDs.erase(ID);
		}
	}

	if (!Bools.empty())
	{
		UPTR CurrIdx = 0;
		_BoolValues.reset(new bool[Bools.size()]);
		for (const auto [ID, DefaultValue] : Bools)
		{
			_Params.emplace(ID, std::pair{ EParamType::Bool, CurrIdx });
			_BoolValues[CurrIdx++] = DefaultValue;

			// Duplicate IDs aren't allowed
			StrIDs.erase(ID);
		}
	}

	if (!StrIDs.empty())
	{
		UPTR CurrIdx = 0;
		_StrIDValues.reset(new CStrID[StrIDs.size()]);
		for (const auto [ID, DefaultValue] : StrIDs)
		{
			_Params.emplace(ID, std::pair{ EParamType::StrID, CurrIdx });
			_StrIDValues[CurrIdx++] = DefaultValue;
		}
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

void CAnimationController::Update(const CSkeleton& Target, float dt)
{
	// update conditions etc

	// TODO:
	// - selector (CStrID based?) with blend time for switching to actions like "open door". Finish vs cancel anim?
	// - pose modifiers = skeletal controls, object space
	// - inertialization
	// - IK

	if (_GraphRoot)
	{
		++_UpdateCounter;
		CAnimationUpdateContext Context{ *this, Target };
		_GraphRoot->Update(Context, dt);

		// TODO: synchronize times by sync group?
	}

	_InertializationDt += dt; // Don't write to _InertializationElapsedTime, because dt may be discarded by teleportation
}
//---------------------------------------------------------------------

void CAnimationController::EvaluatePose(CSkeleton& Target)
{
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

bool CAnimationController::FindParam(CStrID ID, EParamType* pOutType, UPTR* pOutIndex) const
{
	auto It = _Params.find(ID);
	if (It == _Params.cend()) return false;
	if (pOutType) *pOutType = It->second.first;
	if (pOutIndex) *pOutIndex = It->second.second;
	return true;
}
//---------------------------------------------------------------------

bool CAnimationController::SetFloat(CStrID ID, float Value)
{
	EParamType Type;
	UPTR Index;
	if (!FindParam(ID, &Type, &Index) || Type != EParamType::Float) return false;
	_FloatValues[Index] = Value;
	return true;
}
//---------------------------------------------------------------------

float CAnimationController::GetFloat(CStrID ID, float Default) const
{
	EParamType Type;
	UPTR Index;
	if (!FindParam(ID, &Type, &Index) || Type != EParamType::Float) return Default;
	return _FloatValues[Index];
}
//---------------------------------------------------------------------

bool CAnimationController::SetBool(CStrID ID, bool Value)
{
	EParamType Type;
	UPTR Index;
	if (!FindParam(ID, &Type, &Index) || Type != EParamType::Bool) return false;
	_BoolValues[Index] = Value;
	return true;
}
//---------------------------------------------------------------------

bool CAnimationController::GetBool(CStrID ID, bool Default) const
{
	EParamType Type;
	UPTR Index;
	if (!FindParam(ID, &Type, &Index) || Type != EParamType::Bool) return Default;
	return _BoolValues[Index];
}
//---------------------------------------------------------------------

float CAnimationController::GetLocomotionPhaseFromPose(const CSkeleton& Skeleton) const
{
	if (_LeftFootBoneIndex == INVALID_BONE_INDEX || _RightFootBoneIndex == INVALID_BONE_INDEX) return -1.f;

	const auto* pRootNode = Skeleton.GetNode(0);
	const auto* pLeftFootNode = Skeleton.GetNode(_LeftFootBoneIndex);
	const auto* pRightFootNode = Skeleton.GetNode(_RightFootBoneIndex);
	if (!pRootNode || !pLeftFootNode || !pRightFootNode) return -1.f;

	auto ForwardDir = pRootNode->GetWorldMatrix().AxisZ();
	ForwardDir.norm();
	auto SideDir = pRootNode->GetWorldMatrix().AxisX();
	SideDir.norm();

	// Project foot offset onto the locomotion plane (fwd, up) and normalize it to get phase direction
	auto PhaseDir = pLeftFootNode->GetWorldMatrix().Translation() - pRightFootNode->GetWorldMatrix().Translation();
	PhaseDir -= (SideDir * PhaseDir.Dot(SideDir));
	PhaseDir.norm();

	const float CosA = PhaseDir.Dot(ForwardDir);
	const float SinA = (PhaseDir * ForwardDir).Dot(SideDir);

	/* TODO: use ACL/RTM for poses
	const auto Fwd = RootCoordSystem.GetColumn(2);
	acl::Vector4_32 ForwardDir = { static_cast<float>(Fwd[0]), static_cast<float>(Fwd[1]), static_cast<float>(Fwd[2]), 0.0f };
	ForwardDir = acl::vector_normalize3(ForwardDir);

	const auto Side = RootCoordSystem.GetColumn(0);
	acl::Vector4_32 SideDir = { static_cast<float>(Side[0]), static_cast<float>(Side[1]), static_cast<float>(Side[2]), 0.0f };
	SideDir = acl::vector_normalize3(SideDir);

	const auto Offset = acl::vector_sub(LeftFootPositions[i], RightFootPositions[i]);
	const auto ProjectedOffset = acl::vector_sub(Offset, acl::vector_mul(SideDir, acl::vector_dot3(Offset, SideDir)));
	const auto PhaseDir = acl::vector_normalize3(ProjectedOffset);

	const float CosA = acl::vector_dot3(PhaseDir, ForwardDir);
	const float SinA = acl::vector_dot3(acl::vector_cross3(PhaseDir, ForwardDir), SideDir);
	*/

	const float Angle = std::copysignf(std::acosf(CosA) * 180.f / PI, SinA); // Could also use Angle = RadToDeg(std::atan2f(SinA, CosA));

	return 180.f - Angle; // map 180 -> -180 to 0 -> 360
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
