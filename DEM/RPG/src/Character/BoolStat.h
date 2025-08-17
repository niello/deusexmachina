#pragma once
#include <Events/Signal.h>
#include <Data/SerializeToParams.h>

// A character boolean stat value that can be temporarily altered by modifiers

namespace DEM::RPG
{
struct CBoolStatDefinition;

class CBoolStat
{
protected:

	static constexpr U32 BaseValueID = 0; //???need? or only use _pStatDef->DefaultValue?

	CBoolStatDefinition* _pStatDef = nullptr;

	// Usually a status effect instance ID
	std::set<U32> _Enablers;
	std::set<U32> _Blockers;
	std::set<U32> _Immunity;

public:

	Events::CSignal<void(const CBoolStat&)> OnModified;

	CBoolStat() = default;
	CBoolStat(const CBoolStat& Other) : CBoolStat(Other.GetBaseValue()) {}
	CBoolStat(CBoolStat&& Other) = default;
	CBoolStat(bool BaseValue) { SetBaseValue(BaseValue); }

	CBoolStat& operator =(const CBoolStat& Other) { SetBaseValue(Other.GetBaseValue()); return *this; }
	CBoolStat& operator =(CBoolStat&& Other) = default;
	CBoolStat& operator =(float BaseValue) { SetBaseValue(BaseValue); return *this; }

	void SetDesc(CBoolStatDefinition* pStatDef);

	void AddEnabler(U32 SourceID);
	void AddBlocker(U32 SourceID);
	void AddImmunity(U32 SourceID);
	void RemoveModifiers(U32 SourceID);
	void RemoveAllModifiers();

	void SetBaseValue(bool NewBaseValue);
	bool GetBaseValue() const { return _Enablers.find(BaseValueID) != _Enablers.cend(); }
	bool Get() const noexcept;

	operator bool() const noexcept { return Get(); }
};

}

namespace DEM::Serialization
{

template<>
struct ParamsFormat<RPG::CBoolStat>
{
	static inline void Serialize(Data::CData& Output, const RPG::CBoolStat& Value)
	{
		Output = Value.GetBaseValue();
	}

	static inline void Deserialize(const Data::CData& Input, RPG::CBoolStat& Value)
	{
		Value.SetBaseValue(Input);
	}
};

}
