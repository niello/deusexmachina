#pragma once
#ifndef __DEM_L2_AI_LEVEL_H__
#define __DEM_L2_AI_LEVEL_H__

#include <StdDEM.h>
#include <Data/Buffer.h>
#include <AI/ActorFwd.h>
#include <AI/Perception/Stimulus.h>
#include <AI/Navigation/NavData.h>
#include <AI/Navigation/PathRequestQueue.h>

// AI level is an abstract space (i.e. some of location views, like GfxLevel & PhysicsLevel),
// that contains stimuli, AI hints and other AI-related world info. Also AILevel serves as
// a navigation manager.

namespace AI
{
class CStimulus;
class CSensor;

class CAILevel: public Core::CRefCounted
{
protected:

	bbox3							Box;
	CStimulusQT						StimulusQT;		// Quadtree containing stimuli and other AI hints
	nDictionary<float, CNavData>	NavData;		// Mapped to maximum radius of agent

	void		QTNodeUpdateActorsSense(CStimulusQT::CNode* pNode, CActor* pActor, CSensor* pSensor, EClipStatus ClipStatus = InvalidClipStatus);
	CNavData*	GetNavData(float ActorRadius);

public:

	~CAILevel();

	bool			Init(const bbox3& LevelBox, uchar QuadTreeDepth);

	bool			LoadNavMesh(const nString& FileName);
	void			UnloadNavMesh();
	dtNavMesh*		GetNavMesh(float ActorRadius);
	dtNavMeshQuery*	GetSyncNavQuery(float ActorRadius);
	bool			GetAsyncNavQuery(float ActorRadius, dtNavMeshQuery*& pOutQuery, CPathRequestQueue*& pOutQueue);
	void			SwitchNavRegionFlags(CStrID ID, bool Set, ushort Flags, float ActorRadius = 0.f);
	void			SetNavRegionFlags(CStrID ID, ushort Flags, float ActorRadius = 0.f);
	void			ClearNavRegionFlags(CStrID ID, ushort Flags, float ActorRadius = 0.f);
	void			SetNavRegionArea(CStrID ID, uchar Area, float ActorRadius = 0.f);

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
	CNavData* pNav = GetNavData(ActorRadius);
	return pNav ? pNav->pNavMesh : NULL;
}
//---------------------------------------------------------------------

inline dtNavMeshQuery* CAILevel::GetSyncNavQuery(float ActorRadius)
{
	CNavData* pNav = GetNavData(ActorRadius);
	return pNav ? pNav->pNavMeshQuery[0] : NULL;
}
//---------------------------------------------------------------------

inline void CAILevel::SetNavRegionFlags(CStrID ID, ushort Flags, float ActorRadius)
{
	SwitchNavRegionFlags(ID, true, Flags, ActorRadius);
}
//---------------------------------------------------------------------

inline void CAILevel::ClearNavRegionFlags(CStrID ID, ushort Flags, float ActorRadius)
{
	SwitchNavRegionFlags(ID, false, Flags, ActorRadius);
}
//---------------------------------------------------------------------

}

#endif