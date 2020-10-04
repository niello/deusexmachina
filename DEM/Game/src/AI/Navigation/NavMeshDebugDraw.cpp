#include "NavMeshDebugDraw.h"
#include <AI/Navigation/NavMesh.h>
#include <Debug/DebugDraw.h>
#include <DetourDebugDraw.h>

namespace Debug
{

CNavMeshDebugDraw::~CNavMeshDebugDraw()
{
	if (_pQuery) dtFreeNavMeshQuery(_pQuery);
}
//---------------------------------------------------------------------

void CNavMeshDebugDraw::DrawNavMesh(const DEM::AI::CNavMesh& NavMesh)
{
	if (auto pDtNavMesh = NavMesh.GetDetourNavMesh())
	{
		duDebugDrawNavMesh(this, *pDtNavMesh, DU_DRAWNAVMESH_OFFMESHCONS);
		//duDebugDrawNavMeshPolysWithFlags(&DD, *pNavQuery->getAttachedNavMesh(), NAV_FLAG_LOCKED, duRGBA(240, 16, 16, 32));
		//duDebugDrawNavMeshBVTree(&DD, *pNavQuery->getAttachedNavMesh());
	}
}
//---------------------------------------------------------------------

void CNavMeshDebugDraw::DrawNavMeshPolyAt(const DEM::AI::CNavMesh& NavMesh, const vector3& Pos, uint32_t Color)
{
	auto pDtNavMesh = NavMesh.GetDetourNavMesh();
	if (!pDtNavMesh) return;

	const float Extents[3] = { 0.f, NavMesh.GetAgentHeight() * 0.5f, 0.f };
	dtPolyRef Ref;
	float Nearest[3];
	dtQueryFilter Filter;

	if (!_pQuery) _pQuery = dtAllocNavMeshQuery();
	if (_pQuery->getAttachedNavMesh() != pDtNavMesh && dtStatusFailed(_pQuery->init(pDtNavMesh, 16))) return;
	if (dtStatusFailed(_pQuery->findNearestPoly(Pos.v, Extents, &Filter, &Ref, Nearest))) return;
	if (!n_fequal(Pos.x, Nearest[0]) || !n_fequal(Pos.z, Nearest[2])) return;

	duDebugDrawNavMeshPoly(this, *pDtNavMesh, Ref, Color);

	////!!!DBG TMP!
	//static dtPolyRef PrevRef = 0;
	//if (Ref != PrevRef)
	//{
	//	::Sys::DbgOut(("Curr nav poly: " + std::to_string(Ref) + '\n').c_str());
	//	PrevRef = Ref;
	//}
}
//---------------------------------------------------------------------

void CNavMeshDebugDraw::begin(duDebugDrawPrimitives prim, float size)
{
	switch (prim)
	{
		case DU_DRAW_POINTS: _PrimType = Render::Prim_PointList; break;
		case DU_DRAW_LINES:  _PrimType = Render::Prim_LineList; break;
		case DU_DRAW_TRIS:   _PrimType = Render::Prim_TriList; break;
		case DU_DRAW_QUADS:  Sys::Error("CNavMeshDebugDraw::begin() > DU_DRAW_QUADS is not supported for now!"); //Render::Prim_TriList;
		default: Sys::Error("CNavMeshDebugDraw::begin() > unknown primitive type");
	}

	_Size = size;
}
//---------------------------------------------------------------------

void CNavMeshDebugDraw::vertex(const float x, const float y, const float z, unsigned int color)
{
	switch (_PrimType)
	{
		case Render::Prim_PointList: _DebugDraw.DrawPoint(x, y, z, color, _Size); break;
		case Render::Prim_LineList:  _DebugDraw.AddLineVertex(x, y, z, color, _Size); break;
		case Render::Prim_TriList:   _DebugDraw.AddTriangleVertex(x, y, z, color); break;
		default: break;
	}
}
//---------------------------------------------------------------------

}
