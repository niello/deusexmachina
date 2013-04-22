#pragma once
#ifndef __DEM_L2_NAV_MESH_DEBUG_DRAW_H__
#define __DEM_L2_NAV_MESH_DEBUG_DRAW_H__

#include <StdDEM.h>
#include <Render/Render.h>
#include <DebugDraw.h>
#include <util/narray.h>
#include <mathlib/vector.h>

// Debug draw interface implementation for Detour navigation mesh.

//!!!use calls to lowlevel methods of my debug draw!

namespace AI
{

struct CNavMeshDebugDraw: public duDebugDraw
{
	Render::EPrimitiveTopology	PrimType;
	float						Size;

	virtual ~CNavMeshDebugDraw() {}

	virtual void	depthMask(bool state) {}
	virtual void	texture(bool state) { /*We ignore texture*/ }
	virtual void	begin(duDebugDrawPrimitives prim, float size = 1.0f);
	virtual void	vertex(const float* pos, unsigned int color) { vertex(pos[0], pos[1], pos[2], color); }
	virtual void	vertex(const float x, const float y, const float z, unsigned int color);
	virtual void	vertex(const float* pos, unsigned int color, const float* uv) { vertex(pos, color); }
	virtual void	vertex(const float x, const float y, const float z, unsigned int color, const float u, const float v) { vertex(x, y, z, color); }
	virtual void	end() {}
};

}

#endif