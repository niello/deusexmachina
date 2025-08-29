#include "NumericStat.h"
#include <Character/Archetype.h>
#include <Character/CharacterSheet.h> // for the intrusive ptr

namespace DEM::RPG
{

CNumericStat::CNumericStat() = default;
CNumericStat::CNumericStat(const CNumericStat& Other) : CNumericStat(Other.GetBaseValue()) {}
CNumericStat::CNumericStat(CNumericStat&& Other) noexcept = default;
CNumericStat::CNumericStat(float BaseValue) : _BaseValue(BaseValue), _FinalValue(BaseValue) {}
CNumericStat::~CNumericStat() = default;
CNumericStat& CNumericStat::operator =(const CNumericStat& Other) { SetBaseValue(Other.GetBaseValue()); return *this; }
CNumericStat& CNumericStat::operator =(CNumericStat&& Other) = default;
CNumericStat& CNumericStat::operator =(float BaseValue) { SetBaseValue(BaseValue); return *this; }
//---------------------------------------------------------------------

void CNumericStat::SetDesc(CNumericStatDefinition* pStatDef)
{
	if (_pStatDef == pStatDef) return;

	if (_pStatDef && _pStatDef->Formula)
	{
		// ...
		_Dirty = true; //???TODO: could use "base value dirty"

		// discard old subscriptions!
	}

	_pStatDef = pStatDef;

	if (pStatDef && pStatDef->Formula)
	{
		//subscribe on deps!? or at the first evaluation? it is already dirty anyway!
		_Dirty = true; //???TODO: could use "base value dirty"
	}

	//???should make dirty if there is no formula? e.g. min/max may affect calcs!

	//???should immediately clamp _base_ value to min/max? or do that only on direct set and on final value evaluation?
}
//---------------------------------------------------------------------

void CNumericStat::SetSheet(const PCharacterSheet& Sheet)
{
	_Sheet = Sheet;
	_Dirty = true;
}
//---------------------------------------------------------------------

void CNumericStat::AddModifier(EModifierType Type, float Value, U32 SourceID, U16 Priority)
{
	const auto It = std::lower_bound(_Modifiers.cbegin(), _Modifiers.cend(), Priority,
		[](const CModifier& Elm, U16 NewPriority) { return Elm.Priority < NewPriority; });
	_Modifiers.insert(It, CModifier{ Value, SourceID, Priority, Type });
	_Dirty = true;
	OnModified(*this);
}
//---------------------------------------------------------------------

void CNumericStat::RemoveModifiers(U32 SourceID)
{
	auto It = std::remove_if(_Modifiers.begin(), _Modifiers.end(), [SourceID](const auto& Elm) { return Elm.SourceID == SourceID; });
	if (It == _Modifiers.end()) return;

	_Modifiers.erase(It, _Modifiers.end());
	_Dirty = true;
	OnModified(*this);
}
//---------------------------------------------------------------------

void CNumericStat::RemoveAllModifiers()
{
	if (_Modifiers.empty()) return;

	_Modifiers.clear();
	_Dirty = true;
	OnModified(*this);
}
//---------------------------------------------------------------------

void CNumericStat::UpdateFinalValue() const
{
	if (!_Dirty) return;

	_Dirty = false;

	//!!!TODO: && base value dirty!
	if (_pStatDef && _pStatDef->Formula && _Sheet)
	{
		_Sheet->BeginStatTracking();
		auto Result = _pStatDef->Formula(_Sheet.Get());
		auto AccessedStats = _Sheet->EndStatTracking();

		if (Result.valid())
		{
			_BaseValue = Result.get<float>();
		}
		else
		{
			::Sys::Error(Result.get<sol::error>().what());
			_BaseValue = 0.f;
		}

		for (auto* pStat : AccessedStats.NumericStats)
		{
			//!!!TODO: subscribe on AccessedStats changes, invalidate base value
		}

		for (auto* pStat : AccessedStats.BoolStats)
		{
			//!!!TODO: subscribe on AccessedStats changes, invalidate base value
		}
	}

	//???always clamp to Min/MaxBaseValue here? e.g. SetDesc might limit the stat but we may not want to change its value forever.
	//SetDesc itself might be temporary

	_FinalValue = _BaseValue;

	for (size_t RangeStart = 0; RangeStart < _Modifiers.size(); /**/)
	{
		// Find the priority range
		const U16 CurrentPriority = _Modifiers[RangeStart].Priority;
		size_t RangeEnd = RangeStart;
		while (RangeEnd < _Modifiers.size() && _Modifiers[RangeEnd].Priority == CurrentPriority)
			++RangeEnd;

		// Process base-independent modifiers
		std::optional<float> Override;
		float Min = _pStatDef ? _pStatDef->MinFinalValue : std::numeric_limits<float>::lowest();
		float Max = _pStatDef ? _pStatDef->MaxFinalValue : std::numeric_limits<float>::max();
		for (size_t i = RangeStart; i < RangeEnd; ++i)
		{
			const auto& Mod = _Modifiers[i];
			switch (Mod.Type)
			{
				case EModifierType::Override: if (!Override || Mod.Value > *Override) Override = Mod.Value; break;
				case EModifierType::Min:      if (Mod.Value > Min) Min = Mod.Value; break;
				case EModifierType::Max:      if (Mod.Value < Max) Max = Mod.Value; break;
			}
		}

		if (Override)
		{
			// Override prevails over arithmetic modifiers
			_FinalValue = *Override;
		}
		else
		{
			// Accumulate delta using the result of previous priority groups as a base
			float Delta = 0.0f;
			for (size_t i = RangeStart; i < RangeEnd; ++i)
			{
				const auto& Mod = _Modifiers[i];
				switch (Mod.Type)
				{
					case EModifierType::Add: Delta += Mod.Value; break;
					case EModifierType::Mul: Delta += _FinalValue * (Mod.Value - 1.0f); break;
				}
			}

			_FinalValue += Delta;
		}

		_FinalValue = std::clamp(_FinalValue, Min, Max);

		RangeStart = RangeEnd;
	}

	if (_pStatDef)
	{
		switch (_pStatDef->RoundingRule)
		{
			case ERoundingRule::Floor:   _FinalValue = std::floor(_FinalValue); break;
			case ERoundingRule::Ceil:    _FinalValue = std::ceil(_FinalValue); break;
			case ERoundingRule::Nearest: _FinalValue = std::round(_FinalValue); break;
		}
	}
}
//---------------------------------------------------------------------

void CNumericStat::SetBaseValue(float NewBaseValue)
{
	if (_BaseValue == NewBaseValue) return;

	_BaseValue = NewBaseValue;
	_Dirty = true;
	OnModified(*this);

	//???should clamp to Min/MaxBaseValue? or do it only in final value eval? in getter too?
}
//---------------------------------------------------------------------

}

