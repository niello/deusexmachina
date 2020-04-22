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
	}
}
//---------------------------------------------------------------------

void CNavMeshDebugDraw::DrawNavMeshPolyAt(const DEM::AI::CNavMesh& NavMesh, const vector3& Pos, uint32_t Color)
{
	auto pDtNavMesh = NavMesh.GetDetourNavMesh();
	if (!pDtNavMesh) return;

	const float Extents[3] = { 0.f, 1.f, 0.f };
	dtPolyRef Ref;
	float Nearest[3];
	dtQueryFilter Filter;

	if (!_pQuery) _pQuery = dtAllocNavMeshQuery();
	if (dtStatusFailed(_pQuery->init(pDtNavMesh, 16))) return;
	if (dtStatusFailed(_pQuery->findNearestPoly(Pos.v, Extents, &Filter, &Ref, Nearest))) return;

	if (n_fequal(Pos.x, Nearest[0]) && n_fequal(Pos.z, Nearest[2]))
		duDebugDrawNavMeshPoly(this, *pDtNavMesh, Ref, Color);
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
	const float Coeff = 1.f / 255.f;
	vector4 VColor(
		(color & 0xff) * Coeff,				// R
		((color >> 8) & 0xff) * Coeff,		// G
		((color >> 16) & 0xff) * Coeff,		// B
		((color >> 24) & 0xff) * Coeff);	// A

	vector3 Pos(x, y, z);

	switch (_PrimType)
	{
		case Render::Prim_PointList: _DebugDraw.DrawPoint(Pos, _Size, VColor); break;
		case Render::Prim_LineList:  _DebugDraw.AddLineVertex(Pos, VColor); break;
		case Render::Prim_TriList:   _DebugDraw.AddTriangleVertex(Pos, VColor); break;
		default: break;
	}
}
//---------------------------------------------------------------------

}