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
	//as phases must not clear RTs and therefore know are they the first to
	//render into or RT/DS already contains this frame data
	//!!!DBG TMP!
	View.GPU->ClearRenderTarget(*View.RTs[0], vector4(0.1f, 0.7f, 0.1f, 1.f));
	if (View.DSBuffer.IsValidPtr())
		View.GPU->ClearDepthStencilBuffer(*View.DSBuffer, Render::Clear_Depth, 1.f, 0);

	// Global params
	//???move to a separate phase? user then may implement it using knowledge about its global shader params.
	//as RP is not overridable, it is not a good place to reference global param names
	//some Globals phase with an association Const -> Shader param name
	//then find const Render::CEffectConstant* for each used const by name
	
	if (View.GetCamera())
	{
		//!!!in a separate virtual phase can find once on init!
		const Render::CEffectConstant* pConstViewProj = GetGlobalConstant(CStrID("ViewProj"));
		if (pConstViewProj)
		{
			const matrix44& ViewProj = View.GetCamera()->GetViewProjMatrix();
			View.Globals.SetConstantValue(pConstViewProj, 0, ViewProj.m, sizeof(matrix44));
		}
	}

	View.Globals.ApplyConstantBuffers();

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