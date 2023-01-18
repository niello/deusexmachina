#pragma once
#include <Character/ParameterModifier.h>
#include <Events/Signal.h>

// A parameter of a character or an object. Has a base value and can be temporarily modified
// from different sources of modification (eqipment, consumables and areas to name a few).

//!!!!TODO: write serialization and deserialization! Can exploit constructors and assignment operators?
//???can write wrapper "SerializeAs<T>"?!

namespace DEM::RPG
{

template<typename T>
class CModifiableParameter
{
protected:

	std::unique_ptr<CParameterModifier<T>> _FirstModifier;

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

	//???!!!how to remove a certain modifier?! e.g. when the item is unequipped! Need to somehow know the source!
	void AddModifier(std::unique_ptr<CParameterModifier<T>>&& Modifier)
	{
		n_assert_dbg(Modifier && !Modifier->NextModifier);

		//???is modification order important? Now added to the head!
		Modifier->NextModifier = std::move(_FirstModifier);
		_FirstModifier = std::move(Modifier);
	}

	void ClearAllModifiers() { while (_FirstModifier) _FirstModifier = std::move(_FirstModifier->NextModifier); } // Prevent recursion

	void UpdateFinalValue()
	{
		T FinalValue = _BaseValue;

		CParameterModifier<T>* pPrev = nullptr;
		CParameterModifier<T>* pCurr = _FirstModifier.get();
		while (pCurr)
		{
			if (pCurr->Apply(FinalValue))
			{
				pPrev = pCurr;
				pCurr = pCurr->NextModifier.get();
			}
			else
			{
				// Remove an expired modifier from the list
				auto& CurrTail = pPrev ? pPrev->NextModifier : _FirstModifier;
				CurrTail = std::move(CurrTail->NextModifier);
				pCurr = CurrTail.get();
			}
		}

		std::swap(_LastFinalValue, FinalValue);

		//!!!FIXME: after default construction and SetBaseValue, _LastFinalValue is empty and not _BaseValue. False change report!
		//could store a flag 'was calculated', but it is used only once and after that it just takes space.
		//!!!FIXME: in CDamageAbsorption, comparing int[] is always false! False change report!
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
