#pragma once
#include <Events/Signal.h>
#include <Data/SerializeToParams.h>
#include <sol/sol.hpp>

// A character numeric stat value that can be temporarily altered by modifiers.
// Calculations are done with floats. Modifiers of the same priority use the same base value, not results
// of each other's application. The results of the priority group becomes a base value for the next one.

namespace DEM::RPG
{

enum class ERoundingRule : U8
{
	None,
	Floor,
	Ceil,
	Nearest
};

struct CNumericStatDefinition
{
	// formula must use facade for dependency access. separate UpdateBaseValue(Facade& Ctx) for secondaries?
	// TODO: dependency list? stats from our formula, preparsed (Sheet mock that records index methamethod requests?). Subscribe on their OnDirty.
	sol::function Formula;
	float         DefaultBaseValue = 0.f;
	float         MinValue = std::numeric_limits<float>::lowest();
	float         MaxValue = std::numeric_limits<float>::max();
	ERoundingRule RoundingRule = ERoundingRule::None;
};

enum class EModifierType : U8
{
	Add,       // Adds to a base value
	Mul,       // Multiplies a base value
	Min,       // Clamps a base value to be not less than this
	Max,       // Clamps a base value to be not greater than this
	Override   // Overrides a base value, the greater one is chosen on a conflict
};

class CNumericStat
{
protected:

	struct CModifier
	{
		float         Value;
		U32           SourceID; // Usually a status effect instance ID
		U16           Priority;
		EModifierType Type;
	};

	std::vector<CModifier>  _Modifiers; // Sorted by Priority
	CNumericStatDefinition* _pStatDef = nullptr;

	float         _BaseValue = 0.f;
	mutable float _FinalValue = 0.f;
	mutable bool  _Dirty = false;

public:

	Events::CSignal<void(const CNumericStat&)> OnModified;

	CNumericStat() = default;
	CNumericStat(const CNumericStat& Other) : CNumericStat(Other.GetBaseValue()) {}
	CNumericStat(CNumericStat&& Other) = default;
	CNumericStat(float BaseValue) : _BaseValue(BaseValue), _FinalValue(BaseValue) {}

	CNumericStat& operator =(const CNumericStat& Other) { SetBaseValue(Other.GetBaseValue()); return *this; }
	CNumericStat& operator =(CNumericStat&& Other) = default;
	CNumericStat& operator =(float BaseValue) { SetBaseValue(BaseValue); return *this; }

	void  AddModifier(EModifierType Type, float Value, U32 SourceID, U16 Priority);
	void  RemoveModifiers(U32 SourceID);
	void  RemoveAllModifiers();

	void  UpdateFinalValue() const;

	void  SetBaseValue(float NewBaseValue);
	float GetBaseValue() const { return _BaseValue; }
	float Get() const { UpdateFinalValue(); return _FinalValue; }

	template<typename T>
	T     Get() const { return static_cast<T>(Get()); }

	operator float() const noexcept { return Get(); }
};

}

namespace DEM::Serialization
{

template<>
struct ParamsFormat<RPG::CNumericStat>
{
	static inline void Serialize(Data::CData& Output, const RPG::CNumericStat& Value)
	{
		Output = Value.GetBaseValue();
	}

	static inline void Deserialize(const Data::CData& Input, RPG::CNumericStat& Value)
	{
		Value.SetBaseValue(Input);
	}
};

}
