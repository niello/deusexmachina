#pragma once
#ifndef __CIDE_NAV_MESH_H__
#define __CIDE_NAV_MESH_H__

#include <StdDEM.h>
#include <kernel/nprofiler.h>
#include <Recast.h>
#include <DetourNavMeshBuilder.h>

namespace Game
{
	class CEntity;
}

class nClass;
class nMesh2;
class nSceneNode;
class nChunkLodNode;
class nChunkLodTree;

const int MAX_CONVEXVOL_PTS = 12;

// Flags
const ushort NAV_FLAG_NORMAL = 0x01;

// Areas
const uchar NAV_AREA_NAMED = 1;	// Special area type for named areas

#pragma pack(push, 1)
struct CConvexVolume
{
	vector3	Vertices[MAX_CONVEXVOL_PTS];
	int		VertexCount;
	float	MinY;
	float	MaxY;
	uchar	Area;
};

struct COffmeshConnection
{
	vector3	From;
	vector3	To;
	float	Radius;
	ushort	Flags;
	bool	Bidirectional;
	uchar	Area;
};
#pragma pack(pop)

int ConvexHull(const vector3* pts, int npts, int* out);

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

	float					Radius;
	float					Height;
	float					Climb;

	nClass*					pTfmClass;
	nClass*					pCLODClass;
	nClass*					pShapeClass;
	nClass*					pSkinShapeClass;
	nClass*					pSkyClass;

	rcConfig				Cfg;
	CRecastContext			Ctx;
	rcHeightfield*			pHF;
	rcCompactHeightfield*	pCompactHF;
	rcPolyMesh*				pMesh;
	rcPolyMeshDetail*		pMeshDetail;

	dtNavMeshCreateParams	Params;

	static const int MAX_OFFMESH_CONNECTIONS = 256;
	float	OffMeshVerts[MAX_OFFMESH_CONNECTIONS * 3 * 2];
	float	OffMeshRadius[MAX_OFFMESH_CONNECTIONS];
	uchar	OffMeshDir[MAX_OFFMESH_CONNECTIONS];
	uchar	OffMeshArea[MAX_OFFMESH_CONNECTIONS];
	ushort	OffMeshFlags[MAX_OFFMESH_CONNECTIONS];
	uint	OffMeshID[MAX_OFFMESH_CONNECTIONS];
	int		OffMeshCount;

	bool AddGeometryNCT2(nChunkLodNode* pNode, nChunkLodTree* pTree, const matrix44* pTfm = NULL, uchar Area = RC_WALKABLE_AREA);
	bool AddGeometry(nSceneNode* pNode, const matrix44* pTfm = NULL, uchar Area = RC_WALKABLE_AREA);

public:

	CNavMeshBuilder();
	~CNavMeshBuilder() { Cleanup(); }

	bool Init(const rcConfig& Config, float MaxClimb);
	bool AddGeometry(Game::CEntity& Entity, uchar Area = RC_WALKABLE_AREA);
	bool AddGeometry(nMesh2* pMesh, int GroupIdx, bool IsStrip, const matrix44* pTfm = NULL, uchar Area = RC_WALKABLE_AREA);
	bool AddGeometry(const float* pVerts, int VertexCount, const int* pTris, int TriCount, uchar Area = RC_WALKABLE_AREA);
	bool PrepareGeometry(float AgentRadius, float AgentHeight);

	void ApplyConvexVolumeArea(CConvexVolume& Volume);
	void ResetAllArea(uchar NewAreaValue = RC_WALKABLE_AREA);

	bool AddOffmeshConnection(COffmeshConnection& Connection);
	void ClearOffmeshConnections() { OffMeshCount = 0; }

	bool BuildNavMesh(bool MonotonePartitioning = false);
	bool BuildDetailMesh();
	bool GetNavMeshData(uchar*& pOutData, int& OutSize);

	void Cleanup();

	float GetRadius() const { return Radius; }
	float GetHeight() const { return Height; }
};
//=====================================================================

#endif
