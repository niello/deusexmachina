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

void CBlendSpace1D::Init(CAnimationControllerInitContext& Context)
{
	EParamType Type;
	if (!Context.Controller.FindParam(_ParamID, &Type, &_ParamIndex) || Type != EParamType::Float)
		_ParamIndex = INVALID_INDEX;

	if (!_Samples.empty())
	{
		for (auto& Sample : _Samples)
			Sample.Source->Init(Context);

		// Call after initializing sources, letting them to contribute to SkeletonInfo
		if (Context.SkeletonInfo)
			_Blender.Initialize(2, Context.SkeletonInfo->GetNodeCount());
	}
}
//---------------------------------------------------------------------

void CBlendSpace1D::Update(CAnimationController& Controller, float dt, CSyncContext* pSyncContext)
{
	if (_Samples.empty()) return;

	//???use cache if input parameter didn't change?

	// TODO:
	if (pSyncContext) NOT_IMPLEMENTED;

	_pSecond = nullptr;
	if (_Samples.size() == 1)
	{
		_pFirst = _Samples[0].Source.get();
		_pFirst->Update(Controller, dt, nullptr);
	}
	else
	{
		const float Input = Controller.GetFloat(_ParamIndex);

		//???TODO: optionally filter input to make transitions smoother?

		auto It = std::lower_bound(_Samples.cbegin(), _Samples.cend(), Input,
			[](const auto& Elm, float Val) { return Elm.Value < Val; });

		if (It == _Samples.cbegin() || (It != _Samples.cend() && n_fequal(It->Value, Input, SAMPLE_MATCH_TOLERANCE)))
		{
			_pFirst = It->Source.get();
			_pFirst->Update(Controller, dt, nullptr);
		}
		else
		{
			auto PrevIt = std::prev(It);
			if (n_fequal(PrevIt->Value, Input, SAMPLE_MATCH_TOLERANCE) || (It == _Samples.cend() && PrevIt->Value < Input))
			{
				_pFirst = PrevIt->Source.get();
				_pFirst->Update(Controller, dt, nullptr);
			}
			else
			{
				// For convenience make sure _pFirst is always the one with greater weight
				const float BlendFactor = (Input - PrevIt->Value) / (It->Value - PrevIt->Value);
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

				//!!!TODO: calc animation speed to prevent foot sliding (lerp between sample speeds/durations by weight?)
				//!!!out of bounds input may require scaling too!

				//???track normalized time right here and update most weighted animation using blended animation length?

				//???in phase matching normalize phase scalar? to fallback to normalized time for clips that don't support phases.

				CSyncContext LocalSyncContext{ ESyncMethod::NormalizedTime, 0.f };

				_pFirst->Update(Controller, dt, nullptr);
				_pSecond->Update(Controller, dt, &LocalSyncContext);
			}
		}
	}
}
//---------------------------------------------------------------------

void CBlendSpace1D::EvaluatePose(IPoseOutput& Output)
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

		_Blender.EvaluatePose(Output);
	}
	else if (_pFirst) _pFirst->EvaluatePose(Output);
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
