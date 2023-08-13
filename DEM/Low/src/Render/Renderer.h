#pragma once
#include <Core/RTTIBaseClass.h>
#include <Data/Array.h>
#include <Math/Matrix44.h>

// An object that provides an interface to a rendering algorithm which operates on renderables

namespace Data
{
	class CParams;
}

namespace Render
{
class IRenderable;
class CGPUDriver;
class CTechnique;
using PRenderer = std::unique_ptr<class IRenderer>;

class IRenderer: public Core::CRTTIBaseClass
{
	RTTI_CLASS_DECL(Render::IRenderer, Core::CRTTIBaseClass);

public:

	struct CRenderContext
	{
		Render::CGPUDriver*		pGPU = nullptr;
		const Render::CTechnique* const* pShaderTechCache = nullptr;
		vector3					CameraPosition;
		matrix44				ViewProjection;
	};

	virtual bool Init(const Data::CParams& Params) = 0;
	virtual bool BeginRange(const CRenderContext& Context) = 0;
	virtual void Render(const CRenderContext& Context, IRenderable& Renderable/*, UPTR SortingKey*/) = 0;
	virtual void EndRange(const CRenderContext& Context) = 0;
};

}
