#pragma once
#include <StdDEM.h>
#include <AI/ActorFwd.h>
#include <AI/Perception/Stimulus.h>
#include <AI/Navigation/NavData.h>
#include <AI/Navigation/PathRequestQueue.h>

// AI level is an abstract space (i.e. some of location views, like GfxLevel & PhysicsLevel),
// that contains stimuli, AI hints and other AI-related world info. Also AILevel serves as
// a navigation manager.

//!!!autoremove expired stimuli!

namespace Debug
{
	class CDebugDraw;
}

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

	bool			Init(const CAABB& LevelBox, U8 QuadTreeDepth);
	void			RenderDebug(Debug::CDebugDraw& DebugDraw);

	bool			LoadNavMesh(const char* pFileName);
	void			UnloadNavMesh();
	CNavData*		GetNavData(float ActorRadius);
	dtNavMesh*		GetNavMesh(float ActorRadius);
	dtNavMeshQuery*	GetSyncNavQuery(float ActorRadius);
	bool			GetAsyncNavQuery(float ActorRadius, dtNavMeshQuery*& pOutQuery, CPathRequestQueue*& pOutQueue);

	bool			CheckNavRegionFlags(CStrID ID, U16 Flags, bool AllPolys, float ActorRadius = 0.f);
	void			SwitchNavRegionFlags(CStrID ID, bool Set, U16 Flags, float ActorRadius = 0.f);
	void			SetNavRegionFlags(CStrID ID, U16 Flags, float ActorRadius = 0.f);
	void			ClearNavRegionFlags(CStrID ID, U16 Flags, float ActorRadius = 0.f);
	void			SetNavRegionArea(CStrID ID, U8 Area, float ActorRadius = 0.f);

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
	return pNav ? pNav->pNavMesh : nullptr;
}
//---------------------------------------------------------------------

inline dtNavMeshQuery* CAILevel::GetSyncNavQuery(float ActorRadius)
{
	CNavData* pNav = GetNavData(ActorRadius);
	return pNav ? pNav->pNavMeshQuery[0] : nullptr;
}
//---------------------------------------------------------------------

inline void CAILevel::SetNavRegionFlags(CStrID ID, U16 Flags, float ActorRadius)
{
	SwitchNavRegionFlags(ID, true, Flags, ActorRadius);
}
//---------------------------------------------------------------------

inline void CAILevel::ClearNavRegionFlags(CStrID ID, U16 Flags, float ActorRadius)
{
	SwitchNavRegionFlags(ID, false, Flags, ActorRadius);
}
//---------------------------------------------------------------------

}
