#include "AILevel.h"

#include <AI/AIServer.h>
#include <AI/Perception/Sensor.h>
#include <AI/Navigation/NavMeshDebugDraw.h>
#include <IO/Streams/FileStream.h>
#include <Events/EventServer.h>
#include <DetourDebugDraw.h>
#include <Debug/DebugDraw.h>
#include <Game/GameServer.h> // For the debug draw only

namespace AI
{

CAILevel::~CAILevel()
{
	UnloadNavMesh();
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

bool CAILevel::LoadNavMesh(const char* pFileName)
{
	IO::CFileStream File(pFileName);
	if (!File.Open(IO::SAM_READ)) FAIL;

	if (File.Get<int>() != '_NM_') FAIL;
	int Version = File.Get<int>();

	int NMCount = File.Get<int>();
	for (int NMIdx = 0; NMIdx < NMCount; ++NMIdx)
	{
		float Radius = File.Get<float>();
		CNavData& New = NavData.Add(Radius);
		New.AgentRadius = Radius;
		New.AgentHeight = File.Get<float>();
		if (!New.LoadFromStream(File))
		{
			UnloadNavMesh();
			FAIL;
		}
	}

	EventSrv->FireEvent(CStrID("OnNavMeshDataChanged"));
	OK;
}
//---------------------------------------------------------------------

void CAILevel::UnloadNavMesh()
{
	for (UPTR i = 0; i < NavData.GetCount(); ++i)
		NavData.ValueAt(i).Clear();
	NavData.Clear();
	EventSrv->FireEvent(CStrID("OnNavMeshDataChanged"));
}
//---------------------------------------------------------------------

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

CNavData* CAILevel::GetNavData(float ActorRadius)
{
	// NavData is assumed to be sorted by key (agent radius) in ascending order
	for (UPTR i = 0; i < NavData.GetCount(); ++i)
		if (ActorRadius <= NavData.KeyAt(i))
			return &NavData.ValueAt(i);

	return NULL;
}
//---------------------------------------------------------------------

bool CAILevel::GetAsyncNavQuery(float ActorRadius, dtNavMeshQuery*& pOutQuery, CPathRequestQueue*& pOutQueue)
{
	CNavData* pNav = GetNavData(ActorRadius);
	if (!pNav) FAIL;

	//!!!Select least loaded thread...
	UPTR ThreadID = 0;

	pOutQuery = pNav->pNavMeshQuery[ThreadID];
	pOutQueue = AISrv->GetPathQueue(ThreadID);
	OK;
}
//---------------------------------------------------------------------

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

void CAILevel::QTNodeUpdateActorsSense(CStimulusQT::CNode* pNode, CActor* pActor, CSensor* pSensor, EClipStatus EClipStatus)
{
	if (!pNode->GetTotalObjCount()) return;

	if (EClipStatus == Clipped)
	{
		CAABB BBox;
		pNode->GetBounds(BBox);
		BBox.Min.y = Box.Min.y;
		BBox.Max.y = Box.Max.y;
		EClipStatus = pSensor->GetBoxClipStatus(pActor, BBox);
	}

	if (EClipStatus == Outside) return;

	for (UPTR i = 0; i < pNode->Data.GetListCount(); ++i)
		if (pSensor->AcceptsStimulusType(*pNode->Data.GetKeyAt(i)))
		{
			CStimulusListSet::CIterator It = pNode->Data.GetHeadAt(i);
			for (; It; ++It) pSensor->SenseStimulus(pActor, (*It));
		}

	if (pNode->HasChildren())
		for (UPTR i = 0; i < 4; ++i)
			QTNodeUpdateActorsSense(pNode->GetChild(i), pActor, pSensor, EClipStatus);
}
//---------------------------------------------------------------------

void CAILevel::RenderDebug()
{
	// Render the first NavMesh (later render navmesh used by the current actor)
	dtNavMeshQuery* pNavQuery = GetSyncNavQuery(0.f);
	if (pNavQuery)
	{
		CNavMeshDebugDraw DD;
		duDebugDrawNavMesh(&DD, *pNavQuery->getAttachedNavMesh(), DU_DRAWNAVMESH_OFFMESHCONS);
		duDebugDrawNavMeshPolysWithFlags(&DD, *pNavQuery->getAttachedNavMesh(), NAV_FLAG_LOCKED, duRGBA(240, 16, 16, 32));
		//duDebugDrawNavMeshBVTree(&DD, *pNavQuery->getAttachedNavMesh());

		NOT_IMPLEMENTED;
		/*
		if (GameSrv->HasMouseIntersection())
		{
			const float Extents[3] = { 0.f, 1.f, 0.f };
			dtPolyRef Ref;
			const vector3& Pos = GameSrv->GetMousePos3D();
			float Nearest[3];
			pNavQuery->findNearestPoly(Pos.v, Extents, AISrv->GetDebugNavQueryFilter(), &Ref, Nearest);
			if (Pos.x == Nearest[0] && Pos.z == Nearest[2])
			{
				CString Text;
				Text.Format("Nav. poly under mouse: %d\n", Ref);
				DebugDraw->DrawText(Text.CStr(), 0.5f, 0.05f);
				duDebugDrawNavMeshPoly(&DD, *pNavQuery->getAttachedNavMesh(), Ref, duRGBA(224, 224, 224, 32));
			}
		}
		*/
	}
}
//---------------------------------------------------------------------

}