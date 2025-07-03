#pragma once
#include <AI/Blackboard.h>

// An AI agent's parameter that can be set directly or read from a blackboard key

// NB: sizeof(CParameter<T>) == sizeof(std::variant<BBKey, T>), so we chose to keep both side
//     by side instead of switching between them. When BB is in use, Value serves as a default.

namespace DEM::AI
{

template<typename T>
class CParameter
{
protected:

	inline static const auto sidBBKey = CStrID("BBKey");
	inline static const auto sidDefault = CStrID("Default");
	inline static const auto sidValue = CStrID("Value");

	CStrID _BBKey; // TODO: can use HVar? What if key is added or deleted from BB? Make a real handle from HVar, with gen counter? And/or subscribe BB changes?
	T      _Value;

public:

	using TPass = std::conditional_t<DEM::Meta::should_pass_by_value<T>, T, const T&>;

	CParameter(TPass Value = {}) : _Value(Value) {}
	CParameter(T&& Value) noexcept : _Value(std::move(Value)) {}
	CParameter(CStrID BBKey, TPass Default = {}) : _BBKey(BBKey), _Value(Default) {}
	CParameter(CStrID BBKey, T&& Default = {}) : _BBKey(BBKey), _Value(std::move(Default)) {}

	CParameter(const Data::CData& Data)
	{
		if (auto* pBBKey = Data.As<CStrID>())
		{
			// For consistency CStrID is always interpreted as a BB key, even if it is also T
			_BBKey = *pBBKey;
		}
		else 
		{
			bool IsFullForm = false;
			if (auto* pParams = Data.As<Data::PParams>())
			{
				// Read a full non-ambiguous form { BBKey='Key' Default=10 } or { Value=10 }
				auto& Params = **pParams;
				IsFullForm = Params.TryGet(_BBKey, sidBBKey);
				if (IsFullForm)
					Params.TryGet(_Value, sidDefault);
				else
					IsFullForm = Params.TryGet(_Value, sidValue);
			}

			// Try reading a value directly from an argument
			if (!IsFullForm) ParamsFormat::Deserialize(Data, _Value);
		}
	}

	TPass Get(const CBlackboard& Blackboard) const
	{
		return _BBKey ? Blackboard.GetStorage().Get(Blackboard.GetStorage().Find(_BBKey), _Value) : _Value;
	}
};

}
