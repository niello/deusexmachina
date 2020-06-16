#include "Interaction.h"
#include <Game/Interaction/TargetFilter.h>

namespace DEM::Game
{
CInteraction::~CInteraction() = default;

bool CInteraction::AddTarget(PTargetFilter&& Filter, std::string_view CursorImage, U32 Count, bool Optional)
{
	if (!Filter || !Count) return false;

	CTargetRecord Rec;
	Rec.Filter = std::move(Filter);
	Rec.CursorImage = CursorImage;
	Rec.Count = Count;
	Rec.Optional = Optional;
	_Targets.push_back(std::move(Rec));
	return true;
}
//---------------------------------------------------------------------

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
