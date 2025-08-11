#include "NumericStat.h"

namespace DEM::RPG
{

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
		float Min = _pStatDef ? _pStatDef->MinValue : std::numeric_limits<float>::lowest();
		float Max = _pStatDef ? _pStatDef->MaxValue : std::numeric_limits<float>::max();
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
}
//---------------------------------------------------------------------

}

