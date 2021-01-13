#pragma once
#include <Data/Ptr.h>
#include <Data/StringID.h>
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

struct CAnimationControllerInitContext
{
	CAnimationController&        Controller;
	PSkeletonInfo&               SkeletonInfo;
	Resources::CResourceManager& ResourceManager;
	std::map<CStrID, CStrID>     AssetOverrides;
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

	// shared conditions (allow nesting or not? if nested, must control cyclic dependencies and enforce calculation order)
	// NB: each condition, shared or not, must cache its value and recalculate only if used parameter values changed!

public:

	CAnimationController();
	CAnimationController(CAnimationController&&) noexcept;
	CAnimationController& operator =(CAnimationController&&) noexcept;
	~CAnimationController();

	void SetGraphRoot(PAnimGraphNode&& GraphRoot);
	void InitParams(std::map<CStrID, float>&& Floats = {}, std::map<CStrID, int>&& Ints = {}, std::map<CStrID, bool>&& Bools = {}, std::map<CStrID, CStrID>&& StrIDs = {});

	//???default node must skip Update and return reference pose from eval?

	void  Init(Resources::CResourceManager& ResMgr, std::map<CStrID, CStrID> AssetOverrides = {});
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

	const CSkeletonInfo* GetSkeletonInfo() const { return _SkeletonInfo.Get(); }
};

}
