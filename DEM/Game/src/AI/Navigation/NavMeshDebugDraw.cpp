#include "NavMeshDebugDraw.h"
#include <Debug/DebugDraw.h>

namespace Debug
{

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