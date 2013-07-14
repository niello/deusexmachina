#pragma once
#ifndef __CIDE_NAV_MESH_H__
#define __CIDE_NAV_MESH_H__

#include <StdDEM.h>
//#include <kernel/nprofiler.h>
#include <Recast.h>
#include <mathlib//matrix44.h>

namespace Game
{
	class CEntity;
}

class nMesh2;
class nChunkLodNode;
class nChunkLodTree;

class CRecastContext: public rcContext
{
protected:

	//nProfiler Prf[RC_MAX_TIMERS];

	//static const int MAX_MESSAGES = 1000;
	//const char* m_messages[MAX_MESSAGES];
	//int m_messageCount;
	//static const int TEXT_POOL_SIZE = 8000;
	//char m_textPool[TEXT_POOL_SIZE];
	//int m_textPoolSize;

	//virtual void doResetLog();
	//virtual void doLog(const rcLogCategory /*category*/, const char* /*msg*/, const int /*len*/);
	//virtual void doResetTimers();
	//virtual void doStartTimer(const rcTimerLabel /*label*/);
	//virtual void doStopTimer(const rcTimerLabel /*label*/);
	//virtual int doGetAccumulatedTime(const rcTimerLabel /*label*/) const;
	
public:

	//CRecastContext();
	//virtual ~CRecastContext();
	//
	//void		dumpLog(const char* format, ...);
	//int			getLogCount() const;
	//const char*	getLogText(const int i) const;
};
//=====================================================================

class CNavMeshBuilder
{
protected:

	rcConfig				Cfg;
	CRecastContext			Ctx;
	rcHeightfield*			pHF;
	rcCompactHeightfield*	pCompactHF;
	float					Radius;
	float					Height;
	float					Climb;

	static const int MAX_OFFMESH_CONNECTIONS = 256;
	float			OffMeshVerts[MAX_OFFMESH_CONNECTIONS * 3 * 2];
	float			OffMeshRadius[MAX_OFFMESH_CONNECTIONS];
	unsigned char	OffMeshDir[MAX_OFFMESH_CONNECTIONS];
	unsigned char	OffMeshArea[MAX_OFFMESH_CONNECTIONS];
	unsigned short	OffMeshFlags[MAX_OFFMESH_CONNECTIONS];
	unsigned int	OffMeshID[MAX_OFFMESH_CONNECTIONS];
	int				OffMeshCount;

	bool AddGeometryNCT2(nChunkLodNode* pNode, nChunkLodTree* pTree, const matrix44* pTfm = NULL, unsigned char Area = RC_WALKABLE_AREA);

public:

	CNavMeshBuilder(): pHF(NULL), pCompactHF(NULL), OffMeshCount(0) {}
	~CNavMeshBuilder() { Cleanup(); }

	bool Init(const rcConfig& Config, float MaxClimb);
	bool AddGeometry(Game::CEntity& Entity, unsigned char Area = RC_WALKABLE_AREA);
	bool AddGeometry(nMesh2* pMesh, const matrix44* pTfm = NULL, unsigned char Area = RC_WALKABLE_AREA);
	bool AddGeometry(const float* pVerts, int VertexCount, const int* pTris, int TriCount, unsigned char Area = RC_WALKABLE_AREA);
	bool AddOffmeshConnection(const float* pStart, const float* pEnd, float Radius, unsigned char Dir, unsigned char Area, unsigned short Flags);
	bool PrepareGeometry(float AgentRadius, float AgentHeight);
	void ApplyConvexVolumeArea();
	bool Build(unsigned char*& pOutData, int& OutSize, bool MonotonePartitioning = false);
	void Cleanup();
};
//=====================================================================

#endif
