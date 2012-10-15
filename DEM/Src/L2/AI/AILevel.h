#pragma once
#ifndef __DEM_L2_AI_LEVEL_H__
#define __DEM_L2_AI_LEVEL_H__

#include <StdDEM.h>
#include <Data/Buffer.h>
#include <AI/ActorFwd.h>
#include <AI/Perception/Stimulus.h>
#include <AI/Navigation/PathRequestQueue.h>

// AI level is an abstract space (i.e. some of location views, like GfxLevel & PhysicsLevel),
// that contains stimuli, AI hints and other AI-related world info. Also AILevel serves as
// a navigation manager.

static const int MAX_NAV_PATH = 256;
static const int MAX_ITERS_PER_UPDATE = 100;
static const int MAX_PATHQUEUE_NODES = 4096;
static const int MAX_COMMON_NODES = 512;

namespace AI
{
class CStimulus;
class CSensor;

class CAILevel: public Core::CRefCounted
{
protected:

	struct CNavData
	{
		dtNavMesh*		pNavMesh;
		dtNavMeshQuery*	pNavMeshQuery[DEM_THREAD_COUNT]; // [0] is sync, main query

		CNavData();
	};

	bbox3							Box;
	CStimulusQT						StimulusQT;						// Quadtree containing stimuli and other AI hints

	Data::CBuffer					NMFile;
	nDictionary<float, CNavData>	NavData;						// Mapped to maximum radius of agent

	void		QTNodeUpdateActorsSense(CStimulusQT::CNode* pNode, CActor* pActor, CSensor* pSensor, EClipStatus ClipStatus = InvalidClipStatus);
	CNavData*	GetNavDataForRadius(float ActorRadius);

public:

	~CAILevel();

	bool			Init(const bbox3& LevelBox, uchar QuadTreeDepth);

	bool			LoadNavMesh(const nString& FileName);
	bool			RegisterNavMesh(float ActorRadius, dtNavMesh* pNavMesh);
	dtNavMesh*		GetNavMesh(float ActorRadius);
	dtNavMeshQuery*	GetSyncNavQuery(float ActorRadius);
	bool			GetAsyncNavQuery(float ActorRadius, dtNavMeshQuery*& pOutQuery, CPathRequestQueue*& pOutQueue);

	CStimulusNode*	RegisterStimulus(CStimulus* pStimulus);
	CStimulusNode*	UpdateStimulusLocation(CStimulus* pStimulus) { n_assert(pStimulus && pStimulus->GetQuadTreeNode()); return StimulusQT.UpdateObject(pStimulus); }
	void			UpdateStimulusLocation(CStimulusNode*& pStimulusNode) { n_assert(pStimulusNode && pStimulusNode->Object->GetQuadTreeNode()); StimulusQT.UpdateObject(pStimulusNode); }
	void			RemoveStimulus(CStimulus* pStimulus);		//!!!autoremove on expire!
	void			RemoveStimulus(CStimulusNode* pStimulusNode);	//!!!autoremove on expire!
	void			UpdateActorsSense(CActor* pActor, CSensor* pSensor);
};
//---------------------------------------------------------------------

typedef Ptr<CAILevel> PAILevel;

inline void CAILevel::UpdateActorsSense(CActor* pActor, CSensor* pSensor)
{
	QTNodeUpdateActorsSense(StimulusQT.GetRootNode(), pActor, pSensor);
}
//---------------------------------------------------------------------

inline dtNavMesh* CAILevel::GetNavMesh(float ActorRadius)
{
	CNavData* pNav = GetNavDataForRadius(ActorRadius);
	return pNav ? pNav->pNavMesh : NULL;
}
//---------------------------------------------------------------------

inline dtNavMeshQuery* CAILevel::GetSyncNavQuery(float ActorRadius)
{
	CNavData* pNav = GetNavDataForRadius(ActorRadius);
	return pNav ? pNav->pNavMeshQuery[0] : NULL;
}
//---------------------------------------------------------------------

}

#endif