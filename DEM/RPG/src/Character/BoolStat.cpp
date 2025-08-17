#include "BoolStat.h"
#include <Character/Archetype.h>

namespace DEM::RPG
{

void CBoolStat::SetDesc(CBoolStatDefinition* pStatDef)
{
	if (_pStatDef == pStatDef) return;

	_pStatDef = pStatDef;
	OnModified(*this);
}
//---------------------------------------------------------------------

void CBoolStat::AddEnabler(U32 SourceID)
{
	if (_Enablers.insert(SourceID).second)
		OnModified(*this);
}
//---------------------------------------------------------------------

void CBoolStat::AddBlocker(U32 SourceID)
{
	if (_Blockers.insert(SourceID).second)
		OnModified(*this);
}
//---------------------------------------------------------------------

void CBoolStat::AddImmunity(U32 SourceID)
{
	if (_Immunity.insert(SourceID).second)
		OnModified(*this);
}
//---------------------------------------------------------------------

void CBoolStat::RemoveModifiers(U32 SourceID)
{
	n_assert(SourceID != BaseValueID);

	size_t Changed = 0;
	Changed += _Enablers.erase(SourceID);
	Changed += _Blockers.erase(SourceID);
	Changed += _Immunity.erase(SourceID);
	if (Changed) OnModified(*this);
}
//---------------------------------------------------------------------

void CBoolStat::RemoveAllModifiers()
{
	if (_Enablers.empty() && _Blockers.empty() && _Immunity.empty()) return;

	//!!!TODO: consider innate enabler! or really use the value from stat def directly?!

	_Enablers.clear();
	_Blockers.clear();
	_Immunity.clear();
	OnModified(*this);
}
//---------------------------------------------------------------------

void CBoolStat::SetBaseValue(bool NewBaseValue)
{
	const bool Changed = NewBaseValue ? _Enablers.insert(BaseValueID).second : !!_Enablers.erase(BaseValueID);
	if (Changed) OnModified(*this);
}
//---------------------------------------------------------------------

bool CBoolStat::Get() const noexcept
{
	return (!_Enablers.empty() || (_pStatDef && _pStatDef->DefaultValue)) && (_Blockers.empty() || !_Immunity.empty());
}
//---------------------------------------------------------------------

}

