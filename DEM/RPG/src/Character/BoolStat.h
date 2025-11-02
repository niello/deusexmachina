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

	inline static const CStrID BaseValueID;

	CBoolStatDefinition* _pStatDef = nullptr;

	std::set<CStrID> _Enablers;
	std::set<CStrID> _Blockers;
	std::set<CStrID> _Immunity;

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

	void AddEnabler(CStrID SourceID);
	void AddBlocker(CStrID SourceID);
	void AddImmunity(CStrID SourceID);
	void RemoveModifiers(CStrID SourceID);
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
