#pragma once
#include <Render/RenderFwd.h>
#include <Data/StringID.h>
#include <Data/Regions.h>
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

	struct CPickInfo
	{
		UPTR  ObjectUID = INVALID_INDEX;
		U32   TriangleIndex = INVALID_INDEX_T<U32>;
		float Z = 1.f;
	};

	CGPURenderablePicker(CView& View, std::map<Render::EEffectType, CStrID>&& GPUPickEffects);
	~CGPURenderablePicker();

	bool        Init(); //???!!!to the constructor?!
	CPickInfo   Pick(const CView& View, const Data::CRectF& RelRect, const std::pair<Render::IRenderable*, UPTR>* pObjects, U32 ObjectCount, UPTR ShaderTechCacheIndex);

	const auto& GetEffects() const { return _GPUPickEffects; }
};

}
