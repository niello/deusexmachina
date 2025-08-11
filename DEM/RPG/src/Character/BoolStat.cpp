#include "BoolStat.h"

namespace DEM::RPG
{

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
	n_assert(SourceID != InnateID);

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
	const bool Changed = NewBaseValue ? _Enablers.insert(InnateID).second : !!_Enablers.erase(InnateID);
	if (Changed) OnModified(*this);
}
//---------------------------------------------------------------------

}

