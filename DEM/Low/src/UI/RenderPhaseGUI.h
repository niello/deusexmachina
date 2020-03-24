#pragma once
#include <Frame/RenderPhase.h>
#include <UI/UIFwd.h>
#include <UI/CEGUI/DEMShaderWrapper.h>

// Frame rendering phase that draws GUI context

namespace Frame
{

class CRenderPhaseGUI: public CRenderPhase
{
	FACTORY_CLASS_DECL;

private:

	CStrID                   RenderTargetID;
	UI::EDrawMode            DrawMode;

	// FIXME: not good to expose CEGUI here, can fix if tech cache is moved
	// outside a wrapper, otherwise it is too costly to recreate it each frame
	//???ideally store the whole GUI renderer here? or at least its shared ref?
	CEGUI::PDEMShaderWrapper ShaderWrapperTextured;
	CEGUI::PDEMShaderWrapper ShaderWrapperColored;

public:

	virtual ~CRenderPhaseGUI() override;

	virtual bool Init(const CRenderPath& Owner, CGraphicsResourceManager& GfxMgr, CStrID PhaseName, const Data::CParams& Desc) override;
	virtual bool Render(CView& View) override;
};

typedef Ptr<CRenderPhaseGUI> PRenderPhaseGUI;

}
