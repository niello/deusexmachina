#include "BlendSpace2D.h"
#include <Animation/AnimationController.h>
#include <Animation/SkeletonInfo.h>
#include <Math/DelaunayTriangulation.hpp>

namespace DEM::Math
{

template<>
struct CDelaunayInputTraits<DEM::Anim::CBlendSpace2D::CSample>
{
	static vector2 GetPoint(const DEM::Anim::CBlendSpace2D::CSample& Value) { return { Value.XValue, Value.YValue }; }
};

}

namespace DEM::Anim
{

CBlendSpace2D::CBlendSpace2D(CStrID XParamID, CStrID YParamID)
	: _XParamID(XParamID)
	, _YParamID(YParamID)
{
}
//---------------------------------------------------------------------

void CBlendSpace2D::Init(CAnimationControllerInitContext& Context)
{
	EParamType XType, YType;
	if (!Context.Controller.FindParam(_XParamID, &XType, &_XParamIndex) || XType != EParamType::Float)
		_XParamIndex = INVALID_INDEX;
	if (!Context.Controller.FindParam(_YParamID, &YType, &_YParamIndex) || YType != EParamType::Float)
		_YParamIndex = INVALID_INDEX;

	if (!_Samples.empty())
	{
		// TODO: build acceleration structure
		//!!!when sampling, too low weight must be discarded with renormalization! E.g. 0.499+0.499+0.002->0.5+0.5+0.0
		Math::Delaunay2D(_Samples.cbegin(), _Samples.cend(), _Triangles);

		for (auto& Sample : _Samples)
			Sample.Source->Init(Context);

		// Call after initializing sources, letting them to contribute to SkeletonInfo
		if (Context.SkeletonInfo)
			_Blender.Initialize(3, Context.SkeletonInfo->GetNodeCount());
	}
}
//---------------------------------------------------------------------

//!!!DUPLICATED CODE! See 1D!
static inline void AdvanceNormalizedTime(float dt, float AnimLength, float& NormalizedTime)
{
	if (AnimLength)
	{
		NormalizedTime += (dt / AnimLength);
		if (NormalizedTime < 0.f) NormalizedTime += (1.f - std::truncf(NormalizedTime));
		else if (NormalizedTime > 1.f) NormalizedTime -= std::truncf(NormalizedTime);
	}
	else NormalizedTime = 0.f;
}
//---------------------------------------------------------------------

//!!!DUPLICATED CODE! See 1D!
void CBlendSpace2D::UpdateSingleSample(CAnimGraphNode& Node, CAnimationController& Controller, float dt, CSyncContext* pSyncContext)
{
	const ESyncMethod SyncMethod = pSyncContext ? pSyncContext->Method : ESyncMethod::None;
	switch (SyncMethod)
	{
		case ESyncMethod::None:
		{
			AdvanceNormalizedTime(dt, Node.GetAnimationLengthScaled(), _NormalizedTime);
			break;
		}
		case ESyncMethod::NormalizedTime:
		case ESyncMethod::PhaseMatching: //???or get _NormalizedTime from _pFirst current phase?
		{
			_NormalizedTime = pSyncContext->NormalizedTime;
			break;
		}
		default: NOT_IMPLEMENTED; break;
	}

	_pFirst = &Node;
	_pSecond = nullptr;
	_pFirst->Update(Controller, dt, pSyncContext);
}
//---------------------------------------------------------------------

void CBlendSpace2D::Update(CAnimationController& Controller, float dt, CSyncContext* pSyncContext)
{
	if (_Samples.empty()) return;

	const float XInput = Controller.GetFloat(_XParamIndex);
	const float YInput = Controller.GetFloat(_YParamIndex);

	//???use cache if input parameter didn't change?

	//???TODO: optionally filter input to make transitions smoother?
	// then dt *= (BeforeFilter / AfterFilter);

	// Scale animation speed for values outside the sample range
	const float ClampedInput = std::clamp(XInput, _Samples.front().XValue, _Samples.back().XValue);
	if (ClampedInput && ClampedInput != XInput) dt *= (XInput / ClampedInput);

	if (_Samples.size() == 1)
	{
		UpdateSingleSample(*_Samples[0].Source, Controller, dt, pSyncContext);
		return;
	}

	const auto It = std::lower_bound(_Samples.cbegin(), _Samples.cend(), XInput,
		[](const auto& Elm, float Val) { return Elm.XValue < Val; });

	if (It == _Samples.cbegin() || (It != _Samples.cend() && n_fequal(It->XValue, XInput, SAMPLE_MATCH_TOLERANCE)))
	{
		UpdateSingleSample(*It->Source, Controller, dt, pSyncContext);
		return;
	}

	const auto PrevIt = std::prev(It);
	if (n_fequal(PrevIt->XValue, XInput, SAMPLE_MATCH_TOLERANCE) || (It == _Samples.cend() && PrevIt->XValue < XInput))
	{
		UpdateSingleSample(*PrevIt->Source, Controller, dt, pSyncContext);
		return;
	}

	//!!!???TODO: check if samples reference the same animation with the same speed etc!?

	// For convenience make sure _pFirst is always the one with greater weight
	const float BlendFactor = (XInput - PrevIt->XValue) / (It->XValue - PrevIt->XValue);
	if (BlendFactor <= 0.5f)
	{
		_pFirst = PrevIt->Source.get();
		_pSecond = It->Source.get();
		_Blender.SetWeight(0, 1.f - BlendFactor);
		_Blender.SetWeight(1, BlendFactor);
	}
	else
	{
		_pFirst = It->Source.get();
		_pSecond = PrevIt->Source.get();
		_Blender.SetWeight(0, BlendFactor);
		_Blender.SetWeight(1, 1.f - BlendFactor);
	}

	const ESyncMethod SyncMethod = pSyncContext ? pSyncContext->Method : ESyncMethod::None;
	switch (SyncMethod)
	{
		case ESyncMethod::None:
		{
			const float AnimLength = It->Source->GetAnimationLengthScaled() * BlendFactor +
				PrevIt->Source->GetAnimationLengthScaled() * (1.f - BlendFactor);
			AdvanceNormalizedTime(dt, AnimLength, _NormalizedTime);

			CSyncContext LocalSyncContext{ ESyncMethod::NormalizedTime, _NormalizedTime };

			_pFirst->Update(Controller, dt, &LocalSyncContext);

			// Try to synchronize next animations by a locomotion phase
			const float Phase = _pFirst->GetLocomotionPhase();
			if (Phase >= 0.f)
			{
				LocalSyncContext.Method = ESyncMethod::PhaseMatching;
				LocalSyncContext.LocomotionPhase = Phase;
			}

			_pSecond->Update(Controller, dt, &LocalSyncContext);

			break;
		}
		case ESyncMethod::NormalizedTime:
		case ESyncMethod::PhaseMatching:
		{
			_NormalizedTime = pSyncContext->NormalizedTime;
			//???if phase matching, get _NormalizedTime from curr _pFirst phase?

			_pFirst->Update(Controller, dt, pSyncContext);
			_pSecond->Update(Controller, dt, pSyncContext);

			break;
		}
		default: NOT_IMPLEMENTED; break;
	}

	//::Sys::DbgOut("***CBlendSpace2D: time %lf, input %lf, ipol (%d-%d) %lf\n", _NormalizedTime, Input,
	//	std::distance(_Samples.cbegin(), PrevIt), std::distance(_Samples.cbegin(), It), BlendFactor);
}
//---------------------------------------------------------------------

void CBlendSpace2D::EvaluatePose(IPoseOutput& Output)
{
	if (_pSecond)
	{
		//???TODO: try inplace blending in Output instead of preallocated Blender? helps to save
		// memory and may be faster! We don't use priority here anyway, and weights always sum to 1.f.
		//Can use special wrapper output CPoseScaleBiasOutput / CPoseWeightedOutput to apply weights on the fly!
		//!!!can even have single and per-bone weigh variations!
		//???cache locality may suffer if blending in place? scene nodes are scattered around the heap.
		_pFirst->EvaluatePose(*_Blender.GetInput(0));
		_pSecond->EvaluatePose(*_Blender.GetInput(1));
		if (_pThird) _pThird->EvaluatePose(*_Blender.GetInput(2));

		_Blender.EvaluatePose(Output);
	}
	else if (_pFirst) _pFirst->EvaluatePose(Output);
}
//---------------------------------------------------------------------

float CBlendSpace2D::GetAnimationLengthScaled() const
{
	if (!_pFirst) return 0.f;
	if (!_pSecond) return _pFirst->GetAnimationLengthScaled();
	return _pFirst->GetAnimationLengthScaled() * _Blender.GetWeight(0) +
		_pSecond->GetAnimationLengthScaled() * _Blender.GetWeight(1) +
		(_pThird ? _pThird->GetAnimationLengthScaled() * _Blender.GetWeight(2) : 0.f);
}
//---------------------------------------------------------------------

float CBlendSpace2D::GetLocomotionPhase() const
{
	// Always from the most weighted animation, others must be synchronized
	return _pFirst ? _pFirst->GetLocomotionPhase() : std::numeric_limits<float>().lowest();
}
//---------------------------------------------------------------------

// NB: sample value must be different from neighbours at least by SAMPLE_MATCH_TOLERANCE
bool CBlendSpace2D::AddSample(PAnimGraphNode&& Source, float XValue, float YValue)
{
	// TODO: fail if acceleration structure/triangles are not empty, or invalidate them and refresh on next Init/sampling!
	if (!Source) return false;

	for (const auto& Sample : _Samples)
		if (n_fequal(Sample.XValue, XValue, SAMPLE_MATCH_TOLERANCE) && n_fequal(Sample.YValue, YValue, SAMPLE_MATCH_TOLERANCE))
			return false;

	_Samples.push_back({ std::move(Source), XValue, YValue });
	return true;
}
//---------------------------------------------------------------------

}
