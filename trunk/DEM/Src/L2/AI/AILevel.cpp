#include "AILevel.h"

#include <AI/AIServer.h>
#include <AI/Perception/Sensor.h>
#include <DetourCommon.h>
#include <DetourNavMeshQuery.h>
#include <DetourPathCorridor.h>

namespace AI
{

CAILevel::CNavData::CNavData(): pNavMesh(NULL)
{
	memset(pNavMeshQuery, 0, sizeof(pNavMeshQuery));
}
//---------------------------------------------------------------------

CAILevel::~CAILevel()
{
	for (int i = 0; i < NavData.Size(); ++i)
	{
		CNavData& Data = NavData.ValueAtIndex(i);
		for (int j = 0; j < DEM_THREAD_COUNT; ++j)
			if (Data.pNavMeshQuery[j])
				dtFreeNavMeshQuery(Data.pNavMeshQuery[j]);
		if (Data.pNavMesh) dtFreeNavMesh(Data.pNavMesh);
	}
	NavData.Clear();
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

//!!!remove radius hardcode, load all meshes from the data stream!
bool CAILevel::LoadNavMesh(char* pData, DWORD Size)
{
	n_assert(pData && Size);

	dtNavMesh* pNavMesh = dtAllocNavMesh();
	if (pNavMesh &&
		dtStatusSucceed(pNavMesh->init((uchar*)pData, Size, DT_TILE_FREE_DATA)) &&
		RegisterNavMesh(0.3f, pNavMesh))
	{
		OK;
	}

	if (pNavMesh) dtFreeNavMesh(pNavMesh);

	FAIL;
}
//---------------------------------------------------------------------

bool CAILevel::RegisterNavMesh(float ActorRadius, dtNavMesh* pNavMesh)
{
	n_assert(pNavMesh);

	if (NavData.Contains(ActorRadius)) FAIL;

	CNavData New;
	New.pNavMesh = pNavMesh;
	for (int i = 0; i < DEM_THREAD_COUNT; ++i)
	{
		New.pNavMeshQuery[i] = dtAllocNavMeshQuery();
		if (!New.pNavMeshQuery[i] || dtStatusFailed(New.pNavMeshQuery[i]->init(pNavMesh, MAX_COMMON_NODES)))
		{
			for (int j = 0; j <= i; ++j)
				if (New.pNavMeshQuery[j])
					dtFreeNavMeshQuery(New.pNavMeshQuery[j]);
			FAIL;
		}
	}
	NavData.Add(ActorRadius, New);
	
	OK;
}
//---------------------------------------------------------------------

CStimulusNode* CAILevel::RegisterStimulus(CStimulus* pStimulus)
{
	n_assert(pStimulus && !pStimulus->GetQuadTreeNode());
	return StimulusQT.AddObject(pStimulus);
}
//---------------------------------------------------------------------

void CAILevel::RemoveStimulus(CStimulus* pStimulus)
{
	n_assert(pStimulus && pStimulus->GetQuadTreeNode());
	StimulusQT.RemoveObject(pStimulus);
}
//---------------------------------------------------------------------

void CAILevel::RemoveStimulus(CStimulusNode* pStimulusNode)
{
	n_assert(pStimulusNode && pStimulusNode->Object->GetQuadTreeNode());
	StimulusQT.RemoveObject(pStimulusNode);
}
//---------------------------------------------------------------------

void CAILevel::QTNodeUpdateActorsSense(CStimulusQT::CNode* pNode, CActor* pActor, CSensor* pSensor, EClipStatus ClipStatus)
{
	if (!pNode->GetTotalObjCount()) return;

	if (ClipStatus == InvalidClipStatus || ClipStatus == Clipped)
	{
		bbox3 BBox;
		pNode->GetBounds(BBox);
		BBox.vmin.y = Box.vmin.y;
		BBox.vmax.y = Box.vmax.y;
		ClipStatus = pSensor->GetBoxClipStatus(pActor, BBox);
	}

	if (ClipStatus == Outside) return;

	for (int i = 0; i < pNode->Data.GetListCount(); ++i)
		if (pSensor->AcceptsStimulusType(*pNode->Data.GetKeyAt(i)))
		{
			CStimulusListSet::CElement* pCurr = pNode->Data.GetHeadAt(i);
			for (; pCurr; pCurr = pCurr->GetSucc())
				pSensor->SenseStimulus(pActor, pCurr->Object);
		}

	if (pNode->HasChildren())
		for (DWORD i = 0; i < 4; i++)
			QTNodeUpdateActorsSense(pNode->GetChild(i), pActor, pSensor, ClipStatus);
}
//---------------------------------------------------------------------

CAILevel::CNavData* CAILevel::GetNavDataForRadius(float ActorRadius)
{
	// NavData is assumed to be sorted by key (agent radius) in ascending order
	for (int i = 0; i < NavData.Size(); ++i)
		if (ActorRadius <= NavData.KeyAtIndex(i))
			return &NavData.ValueAtIndex(i);

	return NULL;
}
//---------------------------------------------------------------------

bool CAILevel::GetAsyncNavQuery(float ActorRadius, dtNavMeshQuery*& pOutQuery, CPathRequestQueue*& pOutQueue)
{
	CNavData* pNav = GetNavDataForRadius(ActorRadius);
	if (!pNav) FAIL;

	//!!!Select least loaded thread...
	DWORD ThreadID = 0;

	pOutQuery = pNav->pNavMeshQuery[ThreadID];
	pOutQueue = AISrv->GetPathQueue(ThreadID);
	OK;
}
//---------------------------------------------------------------------

} //namespace AI