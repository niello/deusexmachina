#include "AnimationController.h"
#include <Animation/Graph/AnimGraphNode.h>
#include <Animation/SkeletonInfo.h>
#include <Animation/PoseOutput.h>

namespace DEM::Anim
{
CAnimationController::CAnimationController() = default;
CAnimationController::CAnimationController(CAnimationController&&) noexcept = default;
CAnimationController& CAnimationController::operator =(CAnimationController&&) noexcept = default;
CAnimationController::~CAnimationController() = default;

void CAnimationController::Init(PAnimGraphNode&& GraphRoot, Resources::CResourceManager& ResMgr,
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
	_PoseIndex = 2;

	if (_GraphRoot)
	{
		CAnimationInitContext Context{ *this, _SkeletonInfo, ResMgr, AssetOverrides };
		_GraphRoot->Init(Context);
	}

	// TODO: if !_SkeletonInfo here, can issue a warning - no leaf animation data is provided or some assets not resolved
	if (_SkeletonInfo)
	{
		_LastPoses[0].SetSize(_SkeletonInfo->GetNodeCount());
		_LastPoses[1].SetSize(_SkeletonInfo->GetNodeCount());
	}
}
//---------------------------------------------------------------------

void CAnimationController::Update(float dt)
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
		CAnimationUpdateContext Context{ *this };
		_GraphRoot->Update(Context, dt);

		// TODO: synchronize times by sync group?
	}
}
//---------------------------------------------------------------------

void CAnimationController::EvaluatePose(CPoseBuffer& Pose)
{
	if (_PoseIndex > 1)
	{
		// Init both poses from current. Should be used at the first frame and when teleported.
		_PoseIndex = 0;
		_LastPoses[0] = Pose;
		_LastPoses[1] = Pose;
	}
	else
	{
		// Swap current and previous pose buffers
		_PoseIndex ^= 1;
		_LastPoses[_PoseIndex] = Pose;
	}

	if (_GraphRoot) _GraphRoot->EvaluatePose(Pose);
	//???else (if no _GraphRoot) leave as is or reset to refpose?

	//!!!DBG TMP!
	//pseudo root motion processing
	//???diff from ref pose? can also be from the first frame of the animation, but that complicates things
	//???precalculate something in tools to simplify processing here? is possible?
	//Output.SetTranslation(0, vector3::Zero);
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

float CAnimationController::GetPhaseFromPose() const
{
	if (_PoseIndex > 1) return 0.f;

	const auto& PrevPose = _LastPoses[_PoseIndex];

	//!!!need to know L&R foot bone indices! Root is always 0.

	return 0.f;
}
//---------------------------------------------------------------------

}
