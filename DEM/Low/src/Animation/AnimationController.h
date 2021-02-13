#pragma once
#include <Data/Ptr.h>
#include <Data/StringID.h>
#include <Animation/PoseBuffer.h>
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
class IPoseOutput;

enum class EParamType
{
	Float = 0,
	Int,
	Bool,
	StrID,
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
	std::unique_ptr<CStrID[]>                     _StrIDValues;

	U32                                           _UpdateCounter = 0;

	CPoseBuffer                                   _LastPoses[2];
	U8                                            _PoseIndex = 2;

	// shared conditions (allow nesting or not? if nested, must control cyclic dependencies and enforce calculation order)
	// NB: each condition, shared or not, must cache its value and recalculate only if used parameter values changed!

public:

	CAnimationController();
	CAnimationController(CAnimationController&&) noexcept;
	CAnimationController& operator =(CAnimationController&&) noexcept;
	~CAnimationController();

	void  Init(PAnimGraphNode&& GraphRoot, Resources::CResourceManager& ResMgr, std::map<CStrID, float>&& Floats = {}, std::map<CStrID, int>&& Ints = {}, std::map<CStrID, bool>&& Bools = {}, std::map<CStrID, CStrID>&& StrIDs = {}, const std::map<CStrID, CStrID>& AssetOverrides = {});
	void  Update(float dt);
	void  EvaluatePose(IPoseOutput& Output);

	bool  FindParam(CStrID ID, EParamType* pOutType = nullptr, UPTR* pOutIndex = nullptr) const;
	bool  SetFloat(CStrID ID, float Value);
	bool  SetFloat(UPTR Index, float Value) { if (Index == INVALID_INDEX) return false; _FloatValues[Index] = Value; return true; }
	float GetFloat(CStrID ID, float Default = 0.f) const;
	float GetFloat(UPTR Index, float Default = 0.f) const { return (Index == INVALID_INDEX) ? Default : _FloatValues[Index]; }
	// SetInt
	// SetBool
	// SetStrID

	float GetPhaseFromPose() const;

	const CSkeletonInfo* GetSkeletonInfo() const { return _SkeletonInfo.Get(); }
	U32                  GetUpdateIndex() const { return _UpdateCounter; }
};

}
