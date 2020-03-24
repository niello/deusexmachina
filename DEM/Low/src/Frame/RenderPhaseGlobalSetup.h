#pragma once
#include <Frame/RenderPhase.h>
#include <Render/ShaderParamTable.h>

// Performs setup of global shader variables and other frame-wide params.

namespace Frame
{

class CRenderPhaseGlobalSetup: public CRenderPhase
{
	FACTORY_CLASS_DECL;

protected:

	// Global shader params
	Render::CShaderConstantParam ViewProjection;
	Render::CShaderConstantParam CameraPosition;

public:

	//virtual ~CRenderPhaseGlobalSetup() {}

	virtual bool Init(const CRenderPath& Owner, CGraphicsResourceManager& GfxMgr, CStrID PhaseName, const Data::CParams& Desc) override;
	virtual bool Render(CView& View) override;
};

}
