#pragma once
#ifndef __DEM_L2_NAV_MESH_DEBUG_DRAW_H__
#define __DEM_L2_NAV_MESH_DEBUG_DRAW_H__

#include <StdDEM.h>
#include <DebugDraw.h>
#include <gfx2/ngfxserver2.h>

// Debug draw interface implementation for Detour navigation mesh.

namespace AI
{

struct CNavMeshDebugDraw: public duDebugDraw
{
	nGfxServer2::PrimitiveType	PrimType;
	unsigned int				VPerP;
	unsigned int				CurrColor;
	nArray<vector3>				Vertices;
	float						Size;

	virtual ~CNavMeshDebugDraw() {}

	virtual void	depthMask(bool state) {}
	virtual void	texture(bool state) { 	/*We ignore texture*/ }
	virtual void	begin(duDebugDrawPrimitives prim, float size = 1.0f);
	virtual void	vertex(const float* pos, unsigned int color);
	virtual void	vertex(const float x, const float y, const float z, unsigned int color);
	virtual void	vertex(const float* pos, unsigned int color, const float* uv);
	virtual void	vertex(const float x, const float y, const float z, unsigned int color, const float u, const float v);
	virtual void	end();
};

}

#endif