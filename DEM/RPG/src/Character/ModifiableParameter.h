#pragma once
#include <Character/ParameterModifier.h>
#include <Events/Signal.h>

// A parameter of a character or an object. Has a base value and can be temporarily modified
// from different sources of modification (eqipment, consumables and areas to name a few).

namespace DEM::RPG
{

template<typename T>
class CModifiableParameter
{
protected:

	Ptr<CParameterModifier<T>> _FirstModifier;

	T _BaseValue = {};
	T _LastFinalValue = {};

public:

	CModifiableParameter() = default;
	CModifiableParameter(const CModifiableParameter<T>& Other) = delete;
	CModifiableParameter(CModifiableParameter<T>&& Other) = default;
	CModifiableParameter(const T& BaseValue) : _BaseValue(BaseValue), _LastFinalValue(BaseValue) {}
	CModifiableParameter(T&& BaseValue) : _BaseValue(std::move(BaseValue)) { _LastFinalValue = BaseValue; }
	virtual ~CModifiableParameter() { ClearAllModifiers(); }

	CModifiableParameter<T>& operator =(const CModifiableParameter<T>& Other) = delete;
	CModifiableParameter<T>& operator =(CModifiableParameter<T>&& Other) = default;

	Events::CSignal<void(const T&, const T&)> OnChanged; // Args: prev value, new value

	void AddModifier(Ptr<CParameterModifier<T>>&& Modifier)
	{
		n_assert_dbg(Modifier && !Modifier->NextModifier);

		CParameterModifier<T>* pPrev = nullptr;
		CParameterModifier<T>* pCurr = _FirstModifier.Get();
		while (pCurr)
		{
			if (pCurr->IsConnected())
			{
				// Find the insertion place according to priority
				if (pCurr->GetPriority() < Modifier->GetPriority()) break;
				pPrev = pCurr;
				pCurr = pCurr->NextModifier.Get();
			}
			else
			{
				// Remove an expired modifier from the list
				auto& CurrTail = pPrev ? pPrev->NextModifier : _FirstModifier;
				CurrTail = std::move(CurrTail->NextModifier);
				pCurr = CurrTail.Get();
			}
		}

		auto& CurrTail = pPrev ? pPrev->NextModifier : _FirstModifier;
		Modifier->NextModifier = std::move(CurrTail);
		CurrTail = std::move(Modifier);
	}

	void ClearAllModifiers() { while (_FirstModifier) _FirstModifier = std::move(_FirstModifier->NextModifier); } // Prevent recursion

	void UpdateFinalValue()
	{
		T FinalValue = _BaseValue;

		CParameterModifier<T>* pPrev = nullptr;
		CParameterModifier<T>* pCurr = _FirstModifier.Get();
		while (pCurr)
		{
			if (pCurr->IsConnected() && pCurr->Apply(FinalValue))
			{
				pPrev = pCurr;
				pCurr = pCurr->NextModifier.Get();
			}
			else
			{
				// Remove an expired modifier from the list
				auto& CurrTail = pPrev ? pPrev->NextModifier : _FirstModifier;
				CurrTail = std::move(CurrTail->NextModifier);
				pCurr = CurrTail.Get();
			}
		}

		std::swap(_LastFinalValue, FinalValue);

		//!!!FIXME: after default construction and SetBaseValue, _LastFinalValue is empty and not _BaseValue. False change report!
		//could store a flag 'was calculated', but it is used only once and after that it just takes space.
		if (_LastFinalValue != FinalValue) OnChanged(FinalValue, _LastFinalValue);
	}

	void SetBaseValue(const T& NewBaseValue) { _BaseValue = NewBaseValue; }
	void SetBaseValue(T&& NewBaseValue) { _BaseValue = std::move(NewBaseValue); }
	const T& GetBaseValue() const { return _BaseValue; }
	const T& GetFinalValue() { UpdateFinalValue(); return _LastFinalValue; }
	const T& GetLastCalculatedFinalValue() const { return _LastFinalValue; } // NB: may be already not actual

	CModifiableParameter<T>& operator =(const T& BaseValue) { SetBaseValue(BaseValue); return *this; }
	CModifiableParameter<T>& operator =(T&& BaseValue) { SetBaseValue(std::move(BaseValue)); return *this; }
};

}


//!!!DBG TMP!
//!!!!TODO: write serialization and deserialization! Can exploit constructors and assignment operators?
//???can write wrapper "SerializeAs<T>"?! Can do:
// using TValue = T;
// template<T> Serializer<CModifiableParameter<T>> { SerializeAs<T> or SerializeAs<CModifiableParameter::TValue> }
//!!!SerializeAs<T> must work for any serialization format!
//Format must be an archive type, a wrapper must be around it!?
//???or should handle it in Metadata, not in serialization!? Need redesigning the system! Analyze well known reflection and serialization APIs.

//???instead of overriding serialization, need to override member access? Now MemberAccess tries to work with base type, but must instead
//use proxy getter and setter to work as with underlying type! Then serialization will start working as needed too!

/*
#include <Data/SerializeToParams.h>

namespace DEM::Serialization
{

template<typename T>
struct ParamsFormat<RPG::CModifiableParameter<T>>
{
	static inline void Serialize(Data::CData& Output, const RPG::CModifiableParameter<T>& Value)
	{
		Serialize(Output, Value.GetBaseValue());
	}

	static inline void Deserialize(const Data::CData& Input, RPG::CModifiableParameter<T>& Value)
	{
		T BaseValue;
		Deserialize(Input, BaseValue);
		Value.SetBaseValue(std::move(BaseValue)); //!!!???reset final value to base?! deserialization is reconstruction, seems logical!
	}
};

//!!!TODO: binary format!

}
*/
