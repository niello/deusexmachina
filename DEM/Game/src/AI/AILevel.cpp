#include "AILevel.h"

#include <AI/AIServer.h>
#include <AI/Perception/Sensor.h>
#include <AI/Navigation/NavMeshDebugDraw.h>
#include <IO/IOServer.h>
#include <IO/BinaryReader.h>
#include <Events/EventServer.h>
#include <DetourDebugDraw.h>
#include <Debug/DebugDraw.h>

namespace AI
{

CAILevel::~CAILevel()
{
}
//---------------------------------------------------------------------

bool CAILevel::Init(const CAABB& LevelBox, U8 QuadTreeDepth)
{
	Box = LevelBox;
	vector3 Center = LevelBox.Center();
	vector3 Size = LevelBox.Size();
	StimulusQT.Build(Center.x, Center.z, Size.x, Size.z, QuadTreeDepth);
	OK;
}
//---------------------------------------------------------------------

/*
// Checks if all polys or any poly (AllPolys = false) of a nav. region contains specified flags.
// Now it is enough to contain ANY flag, not all.
bool CAILevel::CheckNavRegionFlags(CStrID ID, U16 Flags, bool AllPolys, float ActorRadius)
{
	bool ProcessAll = ActorRadius <= 0.f;
	for (UPTR i = 0; i < NavData.GetCount(); ++i)
		if (ProcessAll || ActorRadius <= NavData.KeyAt(i))
		{
			CNavData& Data = NavData.ValueAt(i);
			CNavRegion* pRegion = Data.Regions.Get(ID);
			if (!pRegion) continue;

			for (UPTR j = 0; j < pRegion->GetCount(); ++j)
			{
				dtPolyRef Ref = (*pRegion)[j];
				U16 PolyFlags;
				if (dtStatusSucceed(Data.pNavMesh->getPolyFlags(Ref, &PolyFlags)))
				{
					if (PolyFlags & Flags)
					{
						if (!AllPolys) OK;
					}
					else if (AllPolys) FAIL;
				}
			}

			if (!ProcessAll) break;
		}

	return AllPolys;
}
//---------------------------------------------------------------------

void CAILevel::SwitchNavRegionFlags(CStrID ID, bool Set, U16 Flags, float ActorRadius)
{
	bool ProcessAll = ActorRadius <= 0.f;
	for (UPTR i = 0; i < NavData.GetCount(); ++i)
		if (ProcessAll || ActorRadius <= NavData.KeyAt(i))
		{
			CNavData& Data = NavData.ValueAt(i);
			CNavRegion* pRegion = Data.Regions.Get(ID);
			if (!pRegion) continue;

			for (UPTR j = 0; j < pRegion->GetCount(); ++j)
			{
				dtPolyRef Ref = (*pRegion)[j];
				U16 PolyFlags;
				if (dtStatusSucceed(Data.pNavMesh->getPolyFlags(Ref, &PolyFlags)))
				{
					if (Set) PolyFlags |= Flags;
					else PolyFlags &= ~Flags;
					Data.pNavMesh->setPolyFlags(Ref, PolyFlags);
				}
			}

			if (!ProcessAll) break;
		}
}
//---------------------------------------------------------------------

void CAILevel::SetNavRegionArea(CStrID ID, U8 Area, float ActorRadius)
{
	bool ProcessAll = ActorRadius <= 0.f;
	for (UPTR i = 0; i < NavData.GetCount(); ++i)
		if (ProcessAll || ActorRadius <= NavData.KeyAt(i))
		{
			CNavData& Data = NavData.ValueAt(i);
			CNavRegion* pRegion = Data.Regions.Get(ID);
			if (!pRegion) continue;

			for (UPTR j = 0; j < pRegion->GetCount(); ++j)
				Data.pNavMesh->setPolyArea((*pRegion)[j], Area);

			if (!ProcessAll) break;
		}
}
//---------------------------------------------------------------------
*/

CStimulusNode CAILevel::RegisterStimulus(CStimulus* pStimulus)
{
	n_assert(pStimulus && !pStimulus->pQTNode);
	CStimulusNode OutNode;
	StimulusQT.AddObject(pStimulus, pStimulus->Position.x, pStimulus->Position.z, pStimulus->Radius, pStimulus->Radius, pStimulus->pQTNode, &OutNode);
	return OutNode;
}
//---------------------------------------------------------------------

CStimulusNode CAILevel::UpdateStimulusLocation(CStimulus* pStimulus)
{
	n_assert(pStimulus && pStimulus->pQTNode);
	CStimulusNode OutNode;
	StimulusQT.UpdateObject(pStimulus, pStimulus->Position.x, pStimulus->Position.z, pStimulus->Radius, pStimulus->Radius, pStimulus->pQTNode, &OutNode);
	return OutNode;
}
//---------------------------------------------------------------------

void CAILevel::UpdateStimulusLocation(CStimulusNode& StimulusNode)
{
	n_assert(StimulusNode && (*StimulusNode)->pQTNode);
	CStimulus* pStimulus = *StimulusNode;
	StimulusQT.UpdateHandle(StimulusNode, pStimulus->Position.x, pStimulus->Position.z, pStimulus->Radius, pStimulus->Radius, pStimulus->pQTNode);
}
//---------------------------------------------------------------------

void CAILevel::QTNodeUpdateActorsSense(CStimulusQT::CNode* pNode, CActor* pActor, CSensor* pSensor, EClipStatus ClipStatus)
{
	if (!pNode->GetTotalObjCount()) return;

	if (ClipStatus == EClipStatus::Clipped)
	{
		CAABB BBox;
		pNode->GetBounds(BBox);
		BBox.Min.y = Box.Min.y;
		BBox.Max.y = Box.Max.y;
		ClipStatus = pSensor->GetBoxClipStatus(pActor, BBox);
	}

	if (ClipStatus == EClipStatus::Outside) return;

	for (UPTR i = 0; i < pNode->Data.GetListCount(); ++i)
		if (pSensor->AcceptsStimulusType(*pNode->Data.GetKeyAt(i)))
		{
			CStimulusListSet::CIterator It = pNode->Data.GetHeadAt(i);
			for (; It; ++It) pSensor->SenseStimulus(pActor, (*It));
		}

	if (pNode->HasChildren())
		for (UPTR i = 0; i < 4; ++i)
			QTNodeUpdateActorsSense(pNode->GetChild(i), pActor, pSensor, ClipStatus);
}
//---------------------------------------------------------------------

void CAILevel::RenderDebug(Debug::CDebugDraw& DebugDraw)
{
	NOT_IMPLEMENTED;
}
//---------------------------------------------------------------------

}
