#pragma once
#include <StdDEM.h>

// Weighted moving average filter. This implementation uses the duration
// of the value as its weight to achieve a constant speed filtering.

// TODO: make common. Use functors!
enum class EInterpolationType
{
	Average,
	Linear,
	Cubic
};

template<typename T = float, EInterpolationType InterpolationType = EInterpolationType::Linear>
class CTimedFilter
{
protected:

	struct CSample
	{
		T     Value = {};
		float Time = 0.f;  // Instead of storing the weight, we store time //???try duration instead of abs time?
	};

	std::vector<CSample> _Samples;
	UPTR                 _CurrIndex = 0; // To use _Samples as a ring buffer
	float                _TimeWindow;
	float                _CurrTime = 0.f;

public:

	CTimedFilter(float TimeWindow = 0.f) : _Samples(8), _TimeWindow(TimeWindow) {}

	void Reset()
	{
		_CurrIndex = 0;
		_CurrTime = 0.f;
		for (auto& Sample : _Samples) Sample.Time = 0.f;
	}

	float Update(float Input, float dt)
	{
		if (_TimeWindow <= 0.f) return Input;

		_CurrTime += dt;

		if (_Samples[_CurrIndex].Value != Input)
		{
			const UPTR Size = _Samples.size();
			UPTR i = 0;
			for (; i < Size; ++i)
			{
				UPTR NewIndex = _CurrIndex + i;
				if (NewIndex >= Size) NewIndex -= Size;

				if (_Samples[NewIndex].Time <= 0.f)
				{
					_CurrIndex = NewIndex;
					break;
				}
			}

			if (i == Size)
			{
				_Samples.push_back({});
				_CurrIndex = Size;
			}

			_Samples[_CurrIndex].Value = Input;
		}

		_Samples[_CurrIndex].Time = _CurrTime;

		float TotalWeight = 0.f;
		float SumInputs = 0.f;
		for (auto& Data : _Samples)
		{
			if (Data.Time <= 0.f) continue;

			const float SampleAge = _CurrTime - Data.Time;
			if (SampleAge > _TimeWindow)
			{
				// Too old to consider
				Data.Time = 0.f;
				continue;
			}

			float Weight = 0.f;
			if constexpr (InterpolationType == EInterpolationType::Average)
				Weight = 1.f;
			else if constexpr (InterpolationType == EInterpolationType::Linear)
				Weight = 1.f - SampleAge / _TimeWindow;
			else if constexpr (InterpolationType == EInterpolationType::Cubic)
				Weight = 1.f - SampleAge * SampleAge * SampleAge / _TimeWindow;

			if (Weight > 0.f)
			{
				SumInputs += Weight * Data.Value;
				TotalWeight += Weight;
			}
		}

		return (TotalWeight > 0.f) ? (SumInputs / TotalWeight) : 0.f;
	}
};
