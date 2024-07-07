#pragma once
#include <Data/Ptr.h>
#include <Data/StringID.h>
#include <Data/VarStorage.h>
#include <Animation/PoseBuffer.h>
#include <Animation/Inertialization.h>
#include <map>

// Animation controller plays an animation graph instance, feeding it with parameters.
// User specifies parameter values as an input and receives a pose as an output.

namespace Resources
{
	class CResourceManager;
}

namespace DEM::Events
{
	class IEventOutput;
}

namespace DEM::Anim
{
using PAnimationController = std::unique_ptr<class CAnimationController>;
using PAnimGraphNode = std::unique_ptr<class CAnimGraphNode>;
using PSkeletonInfo = Ptr<class CSkeletonInfo>;
class CSkeleton;
using CAnimVarStorage = CVarStorage<bool, int, float, CStrID>;

// FIXME: unify EmptyPort, InvalidPort and this!
inline constexpr U16 INVALID_BONE_INDEX = std::numeric_limits<U16>().max();
inline const CStrID Event_AnimEnd("SYSEVENT_AnimEnd");

enum class EParamType
{
	Float = 0,
	Int,
	Bool,
	String,
	Invalid // For inexistent params
};

struct CAnimationInitContext
{
	CAnimationController&           Controller;
	PSkeletonInfo&                  SkeletonInfo;
	Resources::CResourceManager&    ResourceManager;
	const std::map<CStrID, CStrID>& AssetOverrides;
};

struct CAnimationUpdateContext
{
	CAnimationController& Controller;
	const CSkeleton&      Target;
	Events::IEventOutput* pEventOutput = nullptr;

	// TODO: map CStrID -> values, per named sync group?
	//float NormalizedTime = 0.f;
	float                 LocomotionPhase = -1.f; // Any negative -> no phase syncing
	U32                   LocomotionWrapCount = 0;
};

class CAnimationController final
{
protected:

	PAnimGraphNode                                _GraphRoot;
	PSkeletonInfo                                 _SkeletonInfo;

	CAnimVarStorage                               _Params;

	U32                                           _UpdateCounter = 0;

	CPoseBuffer                                   _CurrPose;
	CPoseBuffer                                   _LastPoses[2];
	float                                         _LastPoseDt = 0.f;
	U8                                            _PoseIndex = 2;

	CInertializationPoseDiff                      _InertializationPoseDiff;
	float                                         _InertializationRequest = -1.f;
	float                                         _InertializationDuration = 0.f;
	float                                         _InertializationElapsedTime = 0.f;
	float                                         _InertializationDeficit = 0.f;
	float                                         _InertializationDt = 0.f;

	U16                                           _LeftFootBoneIndex = INVALID_BONE_INDEX;
	U16                                           _RightFootBoneIndex = INVALID_BONE_INDEX;

	// shared conditions (allow nesting or not? if nested, must control cyclic dependencies and enforce calculation order)
	// NB: each condition, shared or not, must cache its value and recalculate only if used parameter values changed!

	void  ProcessInertialization();

public:

	CAnimationController();
	CAnimationController(CAnimationController&&) noexcept;
	CAnimationController& operator =(CAnimationController&&) noexcept;
	~CAnimationController();

	void   Init(PAnimGraphNode&& GraphRoot, Resources::CResourceManager& ResMgr, CStrID LeftFootID = {}, CStrID RightFootID = {}, std::map<CStrID, float>&& Floats = {}, std::map<CStrID, int>&& Ints = {}, std::map<CStrID, bool>&& Bools = {}, std::map<CStrID, CStrID>&& Strings = {}, const std::map<CStrID, CStrID>& AssetOverrides = {});
	void   Update(const CSkeleton& Target, float dt, Events::IEventOutput* pEventOutput);
	void   EvaluatePose(CSkeleton& Target);

	auto&  GetParams() { return _Params; }
	auto&  GetParams() const { return _Params; }

	float  GetLocomotionPhaseFromPose(const CSkeleton& Skeleton) const;
	float  GetExpectedAnimationLength() const;
	void   RequestInertialization(float Duration);

	const CSkeletonInfo* GetSkeletonInfo() const { return _SkeletonInfo.Get(); }
	U32                  GetUpdateIndex() const { return _UpdateCounter; }
};

}
