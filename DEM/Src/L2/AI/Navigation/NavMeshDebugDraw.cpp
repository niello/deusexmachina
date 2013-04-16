#include "NavMeshDebugDraw.h"

namespace AI
{

void CNavMeshDebugDraw::begin(duDebugDrawPrimitives prim, float size)
{
	switch (prim)
	{
		case DU_DRAW_POINTS:	PrimType = Render::PointList; VPerP = 1; break;
		case DU_DRAW_LINES:		PrimType = Render::LineList; VPerP = 2; break;
		case DU_DRAW_TRIS:		PrimType = Render::TriList; VPerP = 3; break;
		case DU_DRAW_QUADS:		n_error("CNavMeshDebugDraw::begin -> DU_DRAW_QUADS is not supported for now!"); //N2PrimType = nGfxServer2::TriangleList;
		default: n_error("CNavMeshDebugDraw::begin -> unknown primitive type");
	}

	Size = size;
	Vertices.Clear();
}
//---------------------------------------------------------------------

void CNavMeshDebugDraw::vertex(const float* pos, unsigned int color)
{
	vertex(pos[0], pos[1], pos[2], color);
}
//---------------------------------------------------------------------

void CNavMeshDebugDraw::vertex(const float x, const float y, const float z, unsigned int color)
{
	int PrimCount = Vertices.Size() / VPerP;

	// We have vertices that form integral number of primitives, and the current color has changed
	if (PrimCount > 0 &&
		CurrColor != color &&
		(Vertices.Size() % (PrimCount * VPerP)) == 0)
	{
		const float Coeff = 1 / 255.f;
		vector4 VColor(
			(CurrColor & 0xff) * Coeff,				// R
			((CurrColor >> 8) & 0xff) * Coeff,		// G
			((CurrColor >> 16) & 0xff) * Coeff,		// B
			((CurrColor >> 24) & 0xff) * Coeff);	// A
		//GFX
		//nGfxServer2::Instance()->DrawShapePrimitives(
		//	PrimType, PrimCount, Vertices.Begin(), 3, matrix44::identity, VColor);
		Vertices.Clear();
		CurrColor = color;
	}
	else if (Vertices.Size() == 0) CurrColor = color;

	vector3* pVert = Vertices.Reserve(1);
	pVert->x = x;
	pVert->y = y;
	pVert->z = z;
}
//---------------------------------------------------------------------

void CNavMeshDebugDraw::vertex(const float* pos, unsigned int color, const float* uv)
{
	// We ignore texture
	vertex(pos, color);
}
//---------------------------------------------------------------------

void CNavMeshDebugDraw::vertex(const float x, const float y, const float z, unsigned int color, const float u, const float v)
{
	// We ignore texture
	vertex(x, y, z, color);
}
//---------------------------------------------------------------------

void CNavMeshDebugDraw::end()
{
	int PrimCount = Vertices.Size() / VPerP;

	if (PrimCount > 0)
	{
		const float Coeff = 1 / 255.f;
		vector4 VColor(
			(CurrColor & 0xff) * Coeff,				// R
			((CurrColor >> 8) & 0xff) * Coeff,		// G
			((CurrColor >> 16) & 0xff) * Coeff,		// B
			((CurrColor >> 24) & 0xff) * Coeff);	// A
		//GFX
		//nGfxServer2::Instance()->DrawShapePrimitives(
		//	PrimType, PrimCount, Vertices.Begin(), 3, matrix44::identity, VColor, Size);
	}

	Vertices.Clear();
}
//---------------------------------------------------------------------

}