#include "BlendSpace1D.h"
#include <Animation/AnimationController.h>

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

		// Map blender to the total skeleton info (after initing child nodes!)
		// Allocate only two blender inputs
	}
}
//---------------------------------------------------------------------

void CBlendSpace1D::Update(CAnimationController& Controller, float dt)
{
	if (_Samples.empty()) return;

	//???use cache if input parameter didn't change?

	_pSecond = nullptr;
	float BlendFactor = 0.f;
	if (_Samples.size() == 1)
	{
		_pFirst = _Samples[0].Source.get();
	}
	else
	{
		const float Input = Controller.GetFloat(_ParamIndex);

		//???TODO: filter input to make transitions smoother?

		auto It = std::lower_bound(_Samples.cbegin(), _Samples.cend(), Input,
			[](const auto& Elm, float Val) { return Elm.Value < Val; });

		if (It != _Samples.cend() && n_fequal(It->Value, Input, SAMPLE_MATCH_TOLERANCE))
		{
			_pFirst = It->Source.get();
		}
		else if (It != _Samples.cbegin())
		{
			auto PrevIt = std::prev(It);
			if (n_fequal(PrevIt->Value, Input, SAMPLE_MATCH_TOLERANCE))
			{
				_pFirst = PrevIt->Source.get();
			}
			else
			{
				_pFirst = PrevIt->Source.get();
				_pSecond = It->Source.get();
				BlendFactor = (Input - PrevIt->Value) / (It->Value - PrevIt->Value);

				//!!!TODO: calc animation speed to prevent foot sliding (lerp between sample speeds/durations by weight?)

				// maybe must do synchronization here, or record info and postpone after graph update
				//???sync only if clip player, or recurse down to players through any nodes?
				//!!!NB: sync may be abs time, normalized time (0 - 1) or by phase matching (calc from feet or manual)!
				//???!!!Phase matching with monotone value may be much better than named markers?
				//Curr time is obtained from the master track!
			}
		}
	}

	_pFirst->Update(Controller, dt);
	if (_pSecond) _pSecond->Update(Controller, dt); //!!!synchronize instead!
}
//---------------------------------------------------------------------

void CBlendSpace1D::EvaluatePose(IPoseOutput& Output)
{
	if (_pSecond)
	{
		// use calculated sources and weights to blend final pose
		// request source poses from sources, must be already updated
		// use local outputs for blending, or passthrough for the first one, and then blend the second one inplace if needed
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
