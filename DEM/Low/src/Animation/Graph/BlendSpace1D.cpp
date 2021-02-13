#include "BlendSpace1D.h"
#include <Animation/AnimationController.h>
#include <Animation/SkeletonInfo.h>

namespace DEM::Anim
{

CBlendSpace1D::CBlendSpace1D(CStrID ParamID)
	: _ParamID(ParamID)
{
}
//---------------------------------------------------------------------

void CBlendSpace1D::Init(CAnimationInitContext& Context)
{
	EParamType Type;
	if (!Context.Controller.FindParam(_ParamID, &Type, &_ParamIndex) || Type != EParamType::Float)
		_ParamIndex = INVALID_INDEX;

	if (!_Samples.empty())
	{
		for (auto& Sample : _Samples)
			Sample.Source->Init(Context);

		// Call after initializing sources, letting them to contribute to SkeletonInfo
		if (Context.SkeletonInfo && _Samples.size() > 1)
			_TmpPose.SetSize(Context.SkeletonInfo->GetNodeCount());
	}
}
//---------------------------------------------------------------------

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

void CBlendSpace1D::Update(CAnimationUpdateContext& Context, float dt)
{
	if (_Samples.empty()) return;

	const float Input = Context.Controller.GetFloat(_ParamIndex);

	//???use cache if input parameter didn't change?

	//???TODO: optionally filter input to make transitions smoother?
	// then dt *= (BeforeFilter / AfterFilter);

	// Scale animation speed for values outside the sample range
	const float ClampedInput = std::clamp(Input, _Samples.front().Value, _Samples.back().Value);
	if (ClampedInput && ClampedInput != Input) dt *= (Input / ClampedInput);

	if (_Samples.size() == 1)
	{
		_Samples[0].Source->Update(Context, dt);
		return;
	}

	const auto It = std::lower_bound(_Samples.cbegin(), _Samples.cend(), Input,
		[](const auto& Elm, float Val) { return Elm.Value < Val; });

	if (It == _Samples.cbegin() || (It != _Samples.cend() && n_fequal(It->Value, Input, SAMPLE_MATCH_TOLERANCE)))
	{
		It->Source->Update(Context, dt);
		return;
	}

	const auto PrevIt = std::prev(It);
	if (n_fequal(PrevIt->Value, Input, SAMPLE_MATCH_TOLERANCE) || (It == _Samples.cend() && PrevIt->Value < Input))
	{
		PrevIt->Source->Update(Context, dt);
		return;
	}

	//!!!???TODO: check if samples reference the same animation with the same speed etc!? or blend speeds?

	_BlendFactor = (Input - PrevIt->Value) / (It->Value - PrevIt->Value);
	_pFirst = PrevIt->Source.get();
	_pSecond = It->Source.get();

	// Update sample playback cursors

	// TODO: playback rate must be correctly calculated for blend spaces (see CRY and UE4?)
	//???need blend space _NormalizedTime? can always get from master animation
	AdvanceNormalizedTime(dt, GetAnimationLengthScaled(), _NormalizedTime);

	_pFirst->Update(Context, dt);
	_pSecond->Update(Context, dt);

	//::Sys::DbgOut("***CBlendSpace1D: time %lf, input %lf, ipol (%d-%d) %lf\n", _NormalizedTime, Input,
	//	std::distance(_Samples.cbegin(), PrevIt), std::distance(_Samples.cbegin(), It), BlendFactor);
}
//---------------------------------------------------------------------

void CBlendSpace1D::EvaluatePose(CPoseBuffer& Output)
{
	if (!_pFirst) return;

	_pFirst->EvaluatePose(Output);

	if (!_pSecond) return;

	Output *= (1.f - _BlendFactor);

	_pSecond->EvaluatePose(_TmpPose);
	_TmpPose *= _BlendFactor;
	Output.Accumulate(_TmpPose);
}
//---------------------------------------------------------------------

float CBlendSpace1D::GetAnimationLengthScaled() const
{
	if (!_pFirst) return 0.f;
	if (!_pSecond) return _pFirst->GetAnimationLengthScaled();
	return _pFirst->GetAnimationLengthScaled() * (1.f - _BlendFactor) +
		_pSecond->GetAnimationLengthScaled() * _BlendFactor;
}
//---------------------------------------------------------------------

// NB: sample value must be different from neighbours at least by SAMPLE_MATCH_TOLERANCE
bool CBlendSpace1D::AddSample(PAnimGraphNode&& Source, float Value)
{
	if (!Source) return false;

	auto It = _Samples.cend();
	if (!_Samples.empty())
	{
		It = std::lower_bound(_Samples.cbegin(), _Samples.cend(), Value,
			[](const auto& Elm, float Val) { return Elm.Value < Val; });

		// Check if the value is the same as upper
		if (It != _Samples.cend() && n_fequal(It->Value, Value, SAMPLE_MATCH_TOLERANCE)) return false;

		// Check if the value is the same as lower
		if (It != _Samples.cbegin() && n_fequal(std::prev(It)->Value, Value, SAMPLE_MATCH_TOLERANCE)) return false;
	}

	_Samples.insert(It, { std::move(Source), Value });
	return true;
}
//---------------------------------------------------------------------

}
