#include "AILevel.h"

#include <AI/AIServer.h>
#include <AI/Perception/Sensor.h>
#include <AI/Navigation/NavMeshDebugDraw.h>
#include <IO/Streams/FileStream.h>
#include <Events/EventManager.h>
#include <DetourDebugDraw.h>

namespace AI
{

CAILevel::~CAILevel()
{
	UnloadNavMesh();
}
//---------------------------------------------------------------------

bool CAILevel::Init(const bbox3& LevelBox, uchar QuadTreeDepth)
{
	Box = LevelBox;
	vector3 Center = LevelBox.center();
	vector3 Size = LevelBox.size();
	StimulusQT.Build(Center.x, Center.z, Size.x, Size.z, QuadTreeDepth);
	OK;
}
//---------------------------------------------------------------------

bool CAILevel::LoadNavMesh(const CString& FileName)
{
	IO::CFileStream File;
	if (!File.Open(FileName, IO::SAM_READ)) FAIL;

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

	EventMgr->FireEvent(CStrID("OnNavMeshDataChanged"));
	OK;
}
//---------------------------------------------------------------------

void CAILevel::UnloadNavMesh()
{
	for (int i = 0; i < NavData.GetCount(); ++i)
		NavData.ValueAt(i).Clear();
	NavData.Clear();
	EventMgr->FireEvent(CStrID("OnNavMeshDataChanged"));
}
//---------------------------------------------------------------------

// Checks if all polys or any poly (AllPolys = false) of a nav. region contains specified flags.
// Now it is enough to contain ANY flag, not all.
bool CAILevel::CheckNavRegionFlags(CStrID ID, ushort Flags, bool AllPolys, float ActorRadius)
{
	bool ProcessAll = ActorRadius <= 0.f;
	for (int i = 0; i < NavData.GetCount(); ++i)
		if (ProcessAll || ActorRadius <= NavData.KeyAt(i))
		{
			CNavData& Data = NavData.ValueAt(i);
			CNavRegion* pRegion = Data.Regions.Get(ID);
			if (!pRegion) continue;

			for (DWORD j = 0; j < pRegion->GetCount(); ++j)
			{
				dtPolyRef Ref = (*pRegion)[j];
				ushort PolyFlags;
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

void CAILevel::SwitchNavRegionFlags(CStrID ID, bool Set, ushort Flags, float ActorRadius)
{
	bool ProcessAll = ActorRadius <= 0.f;
	for (int i = 0; i < NavData.GetCount(); ++i)
		if (ProcessAll || ActorRadius <= NavData.KeyAt(i))
		{
			CNavData& Data = NavData.ValueAt(i);
			CNavRegion* pRegion = Data.Regions.Get(ID);
			if (!pRegion) continue;

			for (DWORD j = 0; j < pRegion->GetCount(); ++j)
			{
				dtPolyRef Ref = (*pRegion)[j];
				ushort PolyFlags;
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

void CAILevel::SetNavRegionArea(CStrID ID, uchar Area, float ActorRadius)
{
	bool ProcessAll = ActorRadius <= 0.f;
	for (int i = 0; i < NavData.GetCount(); ++i)
		if (ProcessAll || ActorRadius <= NavData.KeyAt(i))
		{
			CNavData& Data = NavData.ValueAt(i);
			CNavRegion* pRegion = Data.Regions.Get(ID);
			if (!pRegion) continue;

			for (DWORD j = 0; j < pRegion->GetCount(); ++j)
				Data.pNavMesh->setPolyArea((*pRegion)[j], Area);

			if (!ProcessAll) break;
		}
}
//---------------------------------------------------------------------

CNavData* CAILevel::GetNavData(float ActorRadius)
{
	// NavData is assumed to be sorted by key (agent radius) in ascending order
	for (int i = 0; i < NavData.GetCount(); ++i)
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
	DWORD ThreadID = 0;

	pOutQuery = pNav->pNavMeshQuery[ThreadID];
	pOutQueue = AISrv->GetPathQueue(ThreadID);
	OK;
}
//---------------------------------------------------------------------

CStimulusNode CAILevel::RegisterStimulus(CStimulus* pStimulus)
{
	n_assert(pStimulus && !pStimulus->GetQuadTreeNode());
	return StimulusQT.AddObject(pStimulus);
}
//---------------------------------------------------------------------

void CAILevel::RemoveStimulus(CStimulus* pStimulus)
{
	n_assert(pStimulus && pStimulus->GetQuadTreeNode());
	StimulusQT.RemoveByValue(pStimulus);
}
//---------------------------------------------------------------------

void CAILevel::RemoveStimulus(CStimulusNode StimulusNode)
{
	n_assert(StimulusNode && (*StimulusNode)->GetQuadTreeNode());
	StimulusQT.RemoveByHandle(StimulusNode);
}
//---------------------------------------------------------------------

void CAILevel::QTNodeUpdateActorsSense(CStimulusQT::CNode* pNode, CActor* pActor, CSensor* pSensor, EClipStatus EClipStatus)
{
	if (!pNode->GetTotalObjCount()) return;

	if (EClipStatus == Clipped)
	{
		bbox3 BBox;
		pNode->GetBounds(BBox);
		BBox.vmin.y = Box.vmin.y;
		BBox.vmax.y = Box.vmax.y;
		EClipStatus = pSensor->GetBoxClipStatus(pActor, BBox);
	}

	if (EClipStatus == Outside) return;

	for (DWORD i = 0; i < pNode->Data.GetListCount(); ++i)
		if (pSensor->AcceptsStimulusType(*pNode->Data.GetKeyAt(i)))
		{
			CStimulusListSet::CIterator It = pNode->Data.GetHeadAt(i);
			for (; It; ++It) pSensor->SenseStimulus(pActor, (*It));
		}

	if (pNode->HasChildren())
		for (DWORD i = 0; i < 4; i++)
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
	}
}
//---------------------------------------------------------------------

} //namespace AI