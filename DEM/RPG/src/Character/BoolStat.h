#pragma once
#include <Data/SerializeToParams.h>

// A character boolean stat value that can be temporarily altered by modifiers

namespace DEM::RPG
{

struct CBoolStatDefinition
{
	bool DefaultValue = false; // 'true' adds an innate enabler for this stat
	// no formula, can't come up with an idea of its usage now
	// inverted name for scripts and facade value getter - for cases like IsNotHexed -> IsHexed
};

class CBoolStat
{
protected:

	// Usually a status effect instance ID
	std::set<U32> _Enablers;
	std::set<U32> _Blockers;
	std::set<U32> _Immunity;

public:

	//???need signal? can be used for secondary param calculation? or signal is needed for different notification reasons?

	static constexpr U32 InnateID = 0;

	CBoolStat() = default;
	CBoolStat(const CBoolStat& Other) : CBoolStat(Other.GetBaseValue()) {}
	CBoolStat(CBoolStat&& Other) = default;
	CBoolStat(bool BaseValue) { SetBaseValue(BaseValue); }

	CBoolStat& operator =(const CBoolStat& Other) { SetBaseValue(Other.GetBaseValue()); return *this; }
	CBoolStat& operator =(CBoolStat&& Other) = default;
	CBoolStat& operator =(float BaseValue) { SetBaseValue(BaseValue); return *this; }

	void AddEnabler(U32 SourceID) { _Enablers.insert(SourceID); }
	void AddBlocker(U32 SourceID) { _Blockers.insert(SourceID); }
	void AddImmunity(U32 SourceID) { _Immunity.insert(SourceID); }

	void Remove(U32 SourceID)
	{
		_Enablers.erase(SourceID);
		_Blockers.erase(SourceID);
		_Immunity.erase(SourceID);
	}

	void SetBaseValue(bool NewBaseValue)
	{
		if (NewBaseValue)
			_Enablers.insert(InnateID);
		else
			_Enablers.erase(InnateID);
	}

	bool GetBaseValue() const { return _Enablers.find(InnateID) != _Enablers.cend(); }
	bool Get() const noexcept { return !_Enablers.empty() && (_Blockers.empty() || !_Immunity.empty()); }

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
