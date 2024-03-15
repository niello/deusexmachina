#pragma once
#include <Render/RenderFwd.h>
#include <Data/StringID.h>
#include <map>

// Renders geometry batches, instanced when possible. Uses sorting, lights.
// Batches are designed to minimize shader state switches.

namespace Frame
{
class CRenderPath;
class CView;

class CGPURenderablePicker
{
protected:

	Render::PRenderTarget       _RT;
	Render::PDepthStencilBuffer _DS;
	UPTR                        _ShaderTechCacheIndex = 0;

public:

	~CGPURenderablePicker();

	bool Init(CView& View, std::map<Render::EEffectType, CStrID>&& EffectOverrides);
	bool Render(CView& View);
};

}
