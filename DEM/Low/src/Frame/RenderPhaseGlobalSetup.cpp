#include "RenderPhaseGlobalSetup.h"

#include <Frame/View.h>
#include <Frame/RenderPath.h>
#include <Frame/NodeAttrCamera.h>
#include <Render/ShaderConstant.h>
#include <Data/Params.h>
#include <Core/Factory.h>

namespace Frame
{
__ImplementClass(Frame::CRenderPhaseGlobalSetup, 'PHGS', Frame::CRenderPhase);

bool CRenderPhaseGlobalSetup::Render(CView& View)
{
	//!!!set only when changed, rebind always, if !bound (checked in GPU)!
	if (View.GetCamera())
	{
		if (pConstViewProjection)
		{
			const matrix44& ViewProj = View.GetCamera()->GetViewProjMatrix();
			Render::CConstantBuffer* pCB = View.Globals.RequestBuffer(pConstViewProjection->Const->GetConstantBufferHandle(), pConstViewProjection->ShaderType);
			pConstViewProjection->Const->SetMatrix(*pCB, &ViewProj);
		}

		if (pConstCameraPosition)
		{
			const vector3& EyePos = View.GetCamera()->GetPosition();
			Render::CConstantBuffer* pCB = View.Globals.RequestBuffer(pConstCameraPosition->Const->GetConstantBufferHandle(), pConstCameraPosition->ShaderType);
			pConstCameraPosition->Const->SetFloat(*pCB, EyePos.v, 3);
		}
	}

	View.Globals.CommitChanges();

	OK;
}
//---------------------------------------------------------------------

bool CRenderPhaseGlobalSetup::Init(const CRenderPath& Owner, CStrID PhaseName, const Data::CParams& Desc)
{
	if (!CRenderPhase::Init(Owner, PhaseName, Desc)) FAIL;

	// Cache global shader parameter descriptions

	const auto& GlobalParams = Owner.GetGlobalParamTable();

	CStrID ViewProjectionName = Desc.Get<CStrID>(CStrID("ViewProjectionName"), CStrID::Empty);
	if (ViewProjectionName.IsValid())
		pConstViewProjection = GlobalParams.GetConstantParam(ViewProjectionName);

	CStrID CameraPositionName = Desc.Get<CStrID>(CStrID("CameraPositionName"), CStrID::Empty);
	if (CameraPositionName.IsValid())
		pConstCameraPosition = GlobalParams.GetConstantParam(CameraPositionName);

	OK;
}
//---------------------------------------------------------------------

}
