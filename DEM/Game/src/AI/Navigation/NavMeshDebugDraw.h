#pragma once
#include <Render/RenderFwd.h>
#include <DebugDraw.h> // FIXME: recast/detour includes are not obvious enough

// Debug draw interface implementation for Detour navigation meshes

namespace Debug
{
class CDebugDraw;

class CNavMeshDebugDraw: public duDebugDraw
{
protected:

	Debug::CDebugDraw&         _DebugDraw;
	Render::EPrimitiveTopology _PrimType;
	float                      _Size;

public:

	CNavMeshDebugDraw(Debug::CDebugDraw& DebugDraw) : _DebugDraw(DebugDraw) {}

	virtual void depthMask(bool state) override {}
	virtual void texture(bool state) override { /*We ignore textures*/ }
	virtual void begin(duDebugDrawPrimitives prim, float size) override;
	virtual void vertex(const float* pos, unsigned int color) override { vertex(pos[0], pos[1], pos[2], color); }
	virtual void vertex(const float x, const float y, const float z, unsigned int color) override;
	virtual void vertex(const float* pos, unsigned int color, const float* uv) override { vertex(pos, color); }
	virtual void vertex(const float x, const float y, const float z, unsigned int color, const float u, const float v) override { vertex(x, y, z, color); }
	virtual void end() override {}
};

}
