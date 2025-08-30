#include "NumericStat.h"
#include <Character/Archetype.h>
#include <Character/BoolStat.h>
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
		_DependencyChangedSubs.clear();

	_pStatDef = pStatDef;

	if (_Sheet && _pStatDef && _pStatDef->Formula)
		_BaseDirty = true;

	//???should make dirty if there is no formula? e.g. min/max may affect calcs!

	//???should immediately clamp _base_ value to new min/max? or do that only on direct set and on final value evaluation?
}
//---------------------------------------------------------------------

void CNumericStat::SetSheet(const PCharacterSheet& Sheet)
{
	if (_Sheet == Sheet) return;

	_Sheet = Sheet;
	if (_Sheet && _pStatDef && _pStatDef->Formula)
		_BaseDirty = true;
}
//---------------------------------------------------------------------

void CNumericStat::AddModifier(EModifierType Type, float Value, U32 SourceID, U16 Priority)
{
	const auto It = std::lower_bound(_Modifiers.cbegin(), _Modifiers.cend(), Priority,
		[](const CModifier& Elm, U16 NewPriority) { return Elm.Priority < NewPriority; });
	_Modifiers.insert(It, CModifier{ Value, SourceID, Priority, Type });
	_FinalDirty = true;
	OnModified(*this);
}
//---------------------------------------------------------------------

void CNumericStat::RemoveModifiers(U32 SourceID)
{
	auto It = std::remove_if(_Modifiers.begin(), _Modifiers.end(), [SourceID](const auto& Elm) { return Elm.SourceID == SourceID; });
	if (It == _Modifiers.end()) return;

	_Modifiers.erase(It, _Modifiers.end());
	_FinalDirty = true;
	OnModified(*this);
}
//---------------------------------------------------------------------

void CNumericStat::RemoveAllModifiers()
{
	if (_Modifiers.empty()) return;

	_Modifiers.clear();
	_FinalDirty = true;
	OnModified(*this);
}
//---------------------------------------------------------------------

float CNumericStat::Get() const
{
	if (!_BaseDirty && !_FinalDirty) return _FinalValue;

	// Recalculate the base value of a secondary stat
	if (_BaseDirty)
	{
		n_assert(_Sheet && _pStatDef && _pStatDef->Formula);

		//!!!???TODO: store in map and renew only changed connections?
		_DependencyChangedSubs.clear();

		_Sheet->BeginStatAccessTracking();
		auto Result = _pStatDef->Formula(_Sheet.Get());
		const auto AccessedStats = _Sheet->EndStatAccessTracking();

		if (Result.valid())
		{
			_BaseValue = Result.get<float>();
		}
		else
		{
			::Sys::Error(Result.get<sol::error>().what());
			_BaseValue = 0.f;
		}

		//???or clamp at assignment to _FinalValue below? to keep the original base value intact. 
		_BaseValue = std::clamp(_BaseValue, _pStatDef->MinBaseValue, _pStatDef->MaxBaseValue);

		for (auto* pStat : AccessedStats.NumericStats)
			_DependencyChangedSubs.push_back(pStat->OnModified.Subscribe([this](auto&) {_BaseDirty = true; }));

		for (auto* pStat : AccessedStats.BoolStats)
			_DependencyChangedSubs.push_back(pStat->OnModified.Subscribe([this](auto&) {_BaseDirty = true; }));

		_BaseDirty = false;
	}

	_FinalValue = _BaseValue;

	// Apply modifiers group bu group
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

	// Apply final rounding
	if (_pStatDef)
	{
		switch (_pStatDef->RoundingRule)
		{
			case ERoundingRule::Floor:   _FinalValue = std::floor(_FinalValue); break;
			case ERoundingRule::Ceil:    _FinalValue = std::ceil(_FinalValue); break;
			case ERoundingRule::Nearest: _FinalValue = std::round(_FinalValue); break;
		}
	}

	_FinalDirty = false;

	return _FinalValue;
}
//---------------------------------------------------------------------

void CNumericStat::SetBaseValue(float NewBaseValue)
{
	n_assert(!_pStatDef || !_pStatDef->Formula);

	if (_BaseValue == NewBaseValue) return;

	_BaseValue = NewBaseValue;
	_BaseDirty = false;
	_FinalDirty = true;
	OnModified(*this);

	//???should clamp to Min/MaxBaseValue? or do it only in final value eval? in getter too?
}
//---------------------------------------------------------------------

}

