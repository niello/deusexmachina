#pragma once
#ifndef __DEM_L2_AI_LEVEL_H__
#define __DEM_L2_AI_LEVEL_H__

#include <StdDEM.h>
#include <Data/Buffer.h>
#include <AI/ActorFwd.h>
#include <AI/Perception/Stimulus.h>
#include <AI/Navigation/NavData.h>
#include <AI/Navigation/PathRequestQueue.h>

// AI level is an abstract space (i.e. some of location views, like GfxLevel & PhysWorld),
// that contains stimuli, AI hints and other AI-related world info. Also AILevel serves as
// a navigation manager.

//!!!autoremove expired stimuli!

namespace AI
{
class CStimulus;
class CSensor;

class CAILevel: public Core::CObject
{
protected:

	CAABB					Box;
	CStimulusQT				StimulusQT;		// Quadtree containing stimuli and other AI hints
	CDict<float, CNavData>	NavData;		// Mapped to maximum radius of agent

	void			QTNodeUpdateActorsSense(CStimulusQT::CNode* pNode, CActor* pActor, CSensor* pSensor, EClipStatus EClipStatus = Clipped);

public:

	~CAILevel();

	bool			Init(const CAABB& LevelBox, uchar QuadTreeDepth);
	void			RenderDebug();

	bool			LoadNavMesh(const char* pFileName);
	void			UnloadNavMesh();
	CNavData*		GetNavData(float ActorRadius);
	dtNavMesh*		GetNavMesh(float ActorRadius);
	dtNavMeshQuery*	GetSyncNavQuery(float ActorRadius);
	bool			GetAsyncNavQuery(float ActorRadius, dtNavMeshQuery*& pOutQuery, CPathRequestQueue*& pOutQueue);

	bool			CheckNavRegionFlags(CStrID ID, ushort Flags, bool AllPolys, float ActorRadius = 0.f);
	void			SwitchNavRegionFlags(CStrID ID, bool Set, ushort Flags, float ActorRadius = 0.f);
	void			SetNavRegionFlags(CStrID ID, ushort Flags, float ActorRadius = 0.f);
	void			ClearNavRegionFlags(CStrID ID, ushort Flags, float ActorRadius = 0.f);
	void			SetNavRegionArea(CStrID ID, uchar Area, float ActorRadius = 0.f);

	//bool			GetNearestValidLocation(const CActor& Actor, CStrID NavRegionID, float Range, vector3& OutPos) const; 
	//bool			GetRandomValidLocation(float ActorRadius, const vector3& Center, float Range, vector3& OutPos) const;

	CStimulusNode	RegisterStimulus(CStimulus* pStimulus);
	CStimulusNode	UpdateStimulusLocation(CStimulus* pStimulus);
	void			UpdateStimulusLocation(CStimulusNode& StimulusNode);
	void			UpdateActorSense(CActor* pActor, CSensor* pSensor);
};
//---------------------------------------------------------------------

typedef Ptr<CAILevel> PAILevel;

inline void CAILevel::UpdateActorSense(CActor* pActor, CSensor* pSensor)
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