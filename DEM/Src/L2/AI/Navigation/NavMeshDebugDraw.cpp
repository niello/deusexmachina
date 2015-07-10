#include "NavMeshDebugDraw.h"

#include <Debug/DebugDraw.h>

namespace AI
{

void CNavMeshDebugDraw::begin(duDebugDrawPrimitives prim, float size)
{
	switch (prim)
	{
		case DU_DRAW_POINTS:	PrimType = Render::Prim_PointList; break;
		case DU_DRAW_LINES:		PrimType = Render::Prim_LineList; break;
		case DU_DRAW_TRIS:		PrimType = Render::Prim_TriList; break;
		case DU_DRAW_QUADS:		Sys::Error("CNavMeshDebugDraw::begin -> DU_DRAW_QUADS is not supported for now!"); //N2PrimType = nGfxServer2::TriangleList;
		default: Sys::Error("CNavMeshDebugDraw::begin -> unknown primitive type");
	}

	Size = size;
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

	switch (PrimType)
	{
		case Render::Prim_PointList:	DebugDraw->DrawPoint(Pos, Size, VColor); break;
		case Render::Prim_LineList:		DebugDraw->AddLineVertex(Pos, VColor); break;
		case Render::Prim_TriList:		DebugDraw->AddTriangleVertex(Pos, VColor); break;
	}
}
//---------------------------------------------------------------------

}