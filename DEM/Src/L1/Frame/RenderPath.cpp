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
	// We clear all phases' render targets and DS surfaces at the beginning of
	// the frame, as recommended, especially for SLI. It also serves for a better
	// rendering architecture, as phases must not clear RTs and therefore know
	// whether they are the first writers of these targets this frame.

	for (UPTR i = 0; i < RTSlots.GetCount(); ++i)
	{
		Render::CRenderTarget* pRT = View.RTs[i].GetUnsafe();
		if (pRT) View.GPU->ClearRenderTarget(*pRT, RTSlots[i].ClearValue);
	}

	for (UPTR i = 0; i < DSSlots.GetCount(); ++i)
	{
		Render::CDepthStencilBuffer* pDS = View.DSBuffers[i].GetUnsafe();
		if (pDS)
		{
			const CDepthStencilSlot& Slot = DSSlots[i];
			View.GPU->ClearDepthStencilBuffer(*pDS, Slot.ClearFlags, Slot.DepthClearValue, Slot.StencilClearValue);
		}
	}

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

		//!!!in a separate virtual phase can find once on init!
		const Render::CEffectConstant* pConstEyePos = GetGlobalConstant(CStrID("EyePos"));
		if (pConstEyePos)
		{
			const vector3& EyePos = View.GetCamera()->GetPosition();
			View.Globals.SetConstantValue(pConstEyePos, 0, EyePos.v, sizeof(vector3));
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