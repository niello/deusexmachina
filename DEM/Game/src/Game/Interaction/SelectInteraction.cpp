#include "Interaction.h"

namespace DEM::Game
{

const CInteraction::CTargetRecord* CInteraction::GetTargetRecord(U32 Index) const
{
	U32 Count = 0;
	for (const auto& Rec : _Targets)
	{
		Count += Rec.Count;
		if (Index < Count) return &Rec;
	}
	return nullptr;
}
//---------------------------------------------------------------------

U32 CInteraction::GetMaxTargetCount() const
{
	U32 Count = 0;
	for (const auto& Rec : _Targets)
		Count += Rec.Count;
	return Count;
}
//---------------------------------------------------------------------

const ITargetFilter* CInteraction::GetTargetFilter(U32 Index) const
{
	auto pRec = GetTargetRecord(Index);
	return pRec ? pRec->Filter.get() : nullptr;
}
//---------------------------------------------------------------------

bool CInteraction::IsTargetOptional(U32 Index) const
{
	auto pRec = GetTargetRecord(Index);
	return pRec ? pRec->Optional : true;
}
//---------------------------------------------------------------------

const std::string& CInteraction::GetCursorImageID(U32 Index) const
{
	auto pRec = GetTargetRecord(Index);
	return (pRec && !pRec->CursorImage.empty()) ? pRec->CursorImage : _CursorImage;
}
//---------------------------------------------------------------------

}
