#include "RenderPath.h"

#include <Frame/RenderPhase.h>
#include <Frame/View.h>
#include <Frame/NodeAttrCamera.h>
#include <Render/GPUDriver.h>
#include <Render/ShaderMetadata.h>

namespace Frame
{
__ImplementClassNoFactory(Frame::CRenderPath, Resources::CResourceObject);

CRenderPath::~CRenderPath()
{
	if (pGlobals) n_delete(pGlobals);
}
//---------------------------------------------------------------------

bool CRenderPath::Render(CView& View)
{
	//!!!clear all phases' render targets and DS surfaces at the beginning
	//of the frame, as recommended, especially for SLI, also it helps rendering
	//as pases must not clear RTs and therefore know are they the first to
	//render into or RT already contains this frame data
	//!!!DBG TMP!
	View.GPU->ClearRenderTarget(*View.RTs[0], vector4(0.1f, 0.7f, 0.1f, 1.f));

	//???move to a phase? user then may implement it using knowledge about its global shader params.
	//as RP is not overridable, it is not a good place to reference global param names
	if (View.GetCamera())
	{
		CStrID sidViewProj("ViewProj");
		UPTR i = 0;
		for (; i < Consts.GetCount(); ++i)
			if (Consts[i].ID == sidViewProj) break;
		if (i < Consts.GetCount())
		{
			const matrix44& ViewProj = View.GetCamera()->GetViewProjMatrix();
			const Render::CEffectConstant& Const = Consts[i];
			Render::CConstantBuffer& ViewProjCB = *View.GlobalCBs[Const.BufferHandle];
			//View.GPU->BeginShaderConstants(ViewProjCB);
			//View.GPU->SetShaderConstant(ViewProjCB, Const.Handle, 0, ViewProj.m, sizeof(ViewProj));
			//View.GPU->CommitShaderConstants(ViewProjCB);
			//View.GPU->BindConstantBuffer(Const.ShaderType, Const.BufferHandle, &ViewProjCB);
		}
	}

	for (UPTR i = 0; i < Phases.GetCount(); ++i)
	{
		if (!Phases[i]->Render(View))
		{
			//???clear tmp view data?
			FAIL;
		}
	}

	//???clear tmp view data?

	OK;
}
//---------------------------------------------------------------------

}