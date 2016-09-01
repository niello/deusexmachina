#include "RenderPhaseGlobalSetup.h"

#include <Frame/View.h>
#include <Frame/RenderPath.h>
#include <Frame/NodeAttrCamera.h>
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
			View.Globals.SetConstantValue(pConstViewProjection, 0, ViewProj.m, sizeof(matrix44));
		}

		if (pConstCameraPosition)
		{
			const vector3& EyePos = View.GetCamera()->GetPosition();
			View.Globals.SetConstantValue(pConstCameraPosition, 0, EyePos.v, sizeof(vector3));
		}
	}

	View.Globals.ApplyConstantBuffers();

	OK;
}
//---------------------------------------------------------------------

bool CRenderPhaseGlobalSetup::Init(const CRenderPath& Owner, CStrID PhaseName, const Data::CParams& Desc)
{
	if (!CRenderPhase::Init(Owner, PhaseName, Desc)) FAIL;

	CStrID ViewProjectionName = Desc.Get<CStrID>(CStrID("ViewProjectionName"), CStrID::Empty);
	if (ViewProjectionName.IsValid()) pConstViewProjection = Owner.GetGlobalConstant(ViewProjectionName);
	CStrID CameraPositionName = Desc.Get<CStrID>(CStrID("CameraPositionName"), CStrID::Empty);
	if (CameraPositionName.IsValid()) pConstCameraPosition = Owner.GetGlobalConstant(CameraPositionName);

	OK;
}
//---------------------------------------------------------------------

}
