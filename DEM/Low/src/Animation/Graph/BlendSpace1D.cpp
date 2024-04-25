#include "BlendSpace1D.h"
#include <Animation/AnimationController.h>
#include <Animation/SkeletonInfo.h>
#include <Math/Math.h>

namespace DEM::Anim
{

CBlendSpace1D::CBlendSpace1D(CStrID ParamID, float SmoothTime)
	: _ParamID(ParamID)
	, _Filter(SmoothTime)
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

void CBlendSpace1D::Update(CAnimationUpdateContext& Context, float dt)
{
	if (_Samples.empty()) return;

	const float RawInput = Context.Controller.GetFloat(_ParamIndex);
	const float Input = _Filter.Update(RawInput, dt);
	//???!!! dt *= (RawInput / Input); ?!

	//???use cache if input parameter didn't change?

	// Scale animation speed for values outside the sample range
	const float ClampedInput = std::clamp(Input, _Samples.front().Value, _Samples.back().Value);
	if (ClampedInput && ClampedInput != Input) dt *= (Input / ClampedInput);

	const auto It = std::lower_bound(_Samples.cbegin(), _Samples.cend(), Input,
		[](const auto& Elm, float Val) { return Elm.Value < Val; });

	if (It == _Samples.cbegin() || (It != _Samples.cend() && n_fequal(It->Value, Input, SAMPLE_MATCH_TOLERANCE)))
	{
		_pFirst = It->Source.get();
		_pSecond = nullptr;
		_BlendFactor = 0.f;
	}
	else
	{
		const auto PrevIt = std::prev(It);
		if (n_fequal(PrevIt->Value, Input, SAMPLE_MATCH_TOLERANCE) || (It == _Samples.cend() && PrevIt->Value < Input))
		{
			_pFirst = PrevIt->Source.get();
			_pSecond = nullptr;
			_BlendFactor = 0.f;
		}
		else
		{
			//!!!???TODO: check if samples reference the same animation with the same speed etc!? or blend speeds?
			_pFirst = PrevIt->Source.get();
			_pSecond = It->Source.get();
			_BlendFactor = (Input - PrevIt->Value) / (It->Value - PrevIt->Value);
		}
	}

	//???!!!FIXME: sort by weight descending, like in blend 2D? Locomotion leader must be the first updated animation with locomotion data!

	// Update sample playback cursors

	// TODO: playback rate must be correctly calculated for blend spaces (see CRY and UE4?)
	//???need blend space _NormalizedTime? can always get from master animation
	//AdvanceNormalizedTime(dt, GetAnimationLengthScaled(), _NormalizedTime); // Current time normalized to [0..1]

	_pFirst->Update(Context, dt);
	if (_pSecond) _pSecond->Update(Context, dt);

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
	Output.Accumulate(_TmpPose, _BlendFactor);

	Output.NormalizeRotations();
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
