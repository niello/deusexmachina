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

	Render::PRenderTarget                 _RT;
	Render::PDepthStencilBuffer           _DS;
	std::map<Render::EEffectType, CStrID> _GPUPickEffects;

public:

	CGPURenderablePicker(CView& View, std::map<Render::EEffectType, CStrID>&& GPUPickEffects);
	~CGPURenderablePicker();

	bool Init(); //???!!!to the constructor?!
	bool Pick(const CView& View, float x, float y, const std::pair<Render::IRenderable*, UPTR>* pObjects, U32 ObjectCount, UPTR ShaderTechCacheIndex);

	const auto& GetEffects() const { return _GPUPickEffects; }
};

}
