#include "RenderPath.h"

#include <Frame/RenderPhase.h>
#include <Frame/View.h>
#include <Frame/NodeAttrCamera.h>
#include <Render/GPUDriver.h>
#include <Render/ConstantBuffer.h>
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

			UPTR j = 0;
			for (; j < View.GlobalCBs.GetCount(); ++i)
				if (View.GlobalCBs[j].Handle == Const.BufferHandle) break;

			if (j < View.GlobalCBs.GetCount())
			{
				Render::CConstantBuffer& ViewProjCB = *View.GlobalCBs[j].Buffer;

				if (!ViewProjCB.IsInEditMode())
					View.GPU->BeginShaderConstants(ViewProjCB);
				View.GPU->SetShaderConstant(ViewProjCB, Const.Handle, 0, ViewProj.m, sizeof(ViewProj));
				View.GlobalCBs[j].ShaderTypes |= (1 << Const.ShaderType);
			}
		}
	}

	// Apply and bind globals
	for (UPTR i = 0; i < View.GlobalCBs.GetCount(); ++i)
		for (UPTR j = 0; j < Render::ShaderType_COUNT; ++j)
			if (View.GlobalCBs[i].ShaderTypes & (1 << j))
			{
				Render::CConstantBuffer& Buffer = *View.GlobalCBs[i].Buffer;
				if (Buffer.IsInEditMode())
					View.GPU->CommitShaderConstants(Buffer);
				View.GPU->BindConstantBuffer((Render::EShaderType)j, View.GlobalCBs[i].Handle, &Buffer);
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