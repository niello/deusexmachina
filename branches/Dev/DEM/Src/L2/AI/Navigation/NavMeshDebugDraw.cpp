#include "NavMeshDebugDraw.h"

#include <Render/DebugDraw.h>

namespace AI
{

void CNavMeshDebugDraw::begin(duDebugDrawPrimitives prim, float size)
{
	switch (prim)
	{
		case DU_DRAW_POINTS:	PrimType = Render::PointList; break;
		case DU_DRAW_LINES:		PrimType = Render::LineList; break;
		case DU_DRAW_TRIS:		PrimType = Render::TriList; break;
		case DU_DRAW_QUADS:		n_error("CNavMeshDebugDraw::begin -> DU_DRAW_QUADS is not supported for now!"); //N2PrimType = nGfxServer2::TriangleList;
		default: n_error("CNavMeshDebugDraw::begin -> unknown primitive type");
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
		case Render::PointList:	DebugDraw->DrawPoint(Pos, Size, VColor); break;
		case Render::LineList:	DebugDraw->AddLineVertex(Pos, VColor); break;
		case Render::TriList:	DebugDraw->AddTriangleVertex(Pos, VColor); break;
	}
}
//---------------------------------------------------------------------

}