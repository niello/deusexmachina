#pragma once
#include <Data/Ptr.h>
#include <Data/StringID.h>
#include <Animation/PoseBuffer.h>
#include <Animation/Inertialization.h>
#include <map>

// Animation controller plays an animation graph instance, feeding it with parameters.
// User specifies parameter values as an input and receives a pose as an output.

namespace Resources
{
	class CResourceManager;
}

namespace DEM::Anim
{
using PAnimationController = std::unique_ptr<class CAnimationController>;
using PAnimGraphNode = std::unique_ptr<class CAnimGraphNode>;
using PSkeletonInfo = Ptr<class CSkeletonInfo>;
class CSkeleton;

// FIXME: unify EmptyPort, InvalidPort and this!
inline constexpr U16 INVALID_BONE_INDEX = std::numeric_limits<U16>().max();

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

	// TODO: map CStrID -> values, per named sync group?
	//float NormalizedTime = 0.f;
	float                 LocomotionPhase = -1.f; // Any negative -> no phase syncing
};

class CAnimationController final
{
protected:

	PAnimGraphNode                                _GraphRoot;
	PSkeletonInfo                                 _SkeletonInfo;

	std::map<CStrID, std::pair<EParamType, UPTR>> _Params; // ID -> Type and Index in a value array
	std::unique_ptr<float[]>                      _FloatValues;
	std::unique_ptr<int[]>                        _IntValues;
	std::unique_ptr<bool[]>                       _BoolValues;
	std::unique_ptr<CStrID[]>                     _StringValues;

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
	void   Update(const CSkeleton& Target, float dt);
	void   EvaluatePose(CSkeleton& Target);

	bool   FindParam(CStrID ID, EParamType* pOutType = nullptr, UPTR* pOutIndex = nullptr) const;
	bool   SetFloat(CStrID ID, float Value);
	bool   SetFloat(UPTR Index, float Value) { if (Index == INVALID_INDEX) return false; _FloatValues[Index] = Value; return true; }
	float  GetFloat(CStrID ID, float Default = 0.f) const;
	float  GetFloat(UPTR Index, float Default = 0.f) const { return (Index == INVALID_INDEX) ? Default : _FloatValues[Index]; }
	bool   SetInt(CStrID ID, int Value);
	bool   SetInt(UPTR Index, int Value) { if (Index == INVALID_INDEX) return false; _IntValues[Index] = Value; return true; }
	int    GetInt(CStrID ID, int Default = 0.f) const;
	int    GetInt(UPTR Index, int Default = 0.f) const { return (Index == INVALID_INDEX) ? Default : _IntValues[Index]; }
	bool   SetBool(CStrID ID, bool Value);
	bool   SetBool(UPTR Index, bool Value) { if (Index == INVALID_INDEX) return false; _BoolValues[Index] = Value; return true; }
	bool   GetBool(CStrID ID, bool Default = 0.f) const;
	bool   GetBool(UPTR Index, bool Default = 0.f) const { return (Index == INVALID_INDEX) ? Default : _BoolValues[Index]; }
	bool   SetString(CStrID ID, CStrID Value);
	bool   SetString(UPTR Index, CStrID Value) { if (Index == INVALID_INDEX) return false; _StringValues[Index] = Value; return true; }
	CStrID GetString(CStrID ID, CStrID Default = CStrID::Empty) const;
	CStrID GetString(UPTR Index, CStrID Default = CStrID::Empty) const { return (Index == INVALID_INDEX) ? Default : _StringValues[Index]; }

	float  GetLocomotionPhaseFromPose(const CSkeleton& Skeleton) const;
	void   RequestInertialization(float Duration);

	const CSkeletonInfo* GetSkeletonInfo() const { return _SkeletonInfo.Get(); }
	U32                  GetUpdateIndex() const { return _UpdateCounter; }
};

}
