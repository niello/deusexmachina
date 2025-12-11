#pragma once
#include <Events/Signal.h>
#include <Data/SerializeToParams.h>

// A character numeric stat value that can be temporarily altered by modifiers.
// Calculations are done with floats. Modifiers of the same priority use the same base value, not results
// of each other's application. The results of the priority group becomes a base value for the next one.
// The base value can be set directly, and if doesn't, it will be initialized from CNumericStatDefinition.

namespace DEM::RPG
{
struct CNumericStatDefinition;
using PCharacterSheet = Ptr<class CCharacterSheet>;

enum class EModifierType : U8
{
	Add,       // Adds to a base value
	Mul,       // Multiplies a base value
	Min,       // Clamps a base value to be not less than this
	Max,       // Clamps a base value to be not greater than this
	Override   // Overrides a base value, the greater one is chosen on a conflict
};

class CNumericStat final
{
protected:

	struct CModifier
	{
		CStrID        SourceID; // Status effect ID, ability ID, equipment slot ID etc
		float         Value;
		U16           Priority;
		EModifierType Type;
	};

	// They go first intentionally, for convenience in the debug watch. Alignment is still perfect.
	mutable float _BaseValue = 0.f;
	mutable float _FinalValue = 0.f;

	std::vector<CModifier>        _Modifiers;          // Sorted by Priority
	const CNumericStatDefinition* _pStatDef = nullptr;
	PCharacterSheet               _Sheet;              // Only for the secondary stat formula evaluation

	mutable std::vector<DEM::Events::CConnection> _DependencyChangedSubs;

	mutable bool  _BaseDirty = true;
	mutable bool  _FinalDirty = false;

public:

	Events::CSignal<void(const CNumericStat&)> OnModified;

	CNumericStat();
	CNumericStat(const CNumericStat& Other);
	CNumericStat(CNumericStat&& Other) noexcept;
	CNumericStat(float BaseValue);
	~CNumericStat();

	CNumericStat& operator =(const CNumericStat& Other);
	CNumericStat& operator =(CNumericStat&& Other);
	CNumericStat& operator =(float BaseValue);

	void  SetDesc(const CNumericStatDefinition* pStatDef);
	void  SetSheet(const PCharacterSheet& Sheet);
	auto* GetDesc() const { return _pStatDef; }

	void  AddModifier(EModifierType Type, float Value, CStrID SourceID, U16 Priority);
	bool  RemoveModifiers(CStrID SourceID);
	void  RemoveAllModifiers();

	void  SetBaseValue(float NewBaseValue);
	float GetBaseValue() const { return _BaseValue; }
	float Get() const;

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
		//!!!TODO: need autoconversion of compatible types!
		if (auto* pInput = Input.As<float>())
			Value.SetBaseValue(*pInput);
		else if (auto* pInput = Input.As<int>())
			Value.SetBaseValue(static_cast<float>(*pInput));
	}
};

}
