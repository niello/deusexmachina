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
		View.Globals.SetMatrix(ViewProjection, View.GetCamera()->GetViewProjMatrix());
		View.Globals.SetVector(CameraPosition, View.GetCamera()->GetPosition());
	}

	View.Globals.CommitChanges();
	//???View.Globals.Apply(); , commit buffers inside?

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
		ViewProjection = GlobalParams.GetConstant(ViewProjectionName);

	CStrID CameraPositionName = Desc.Get<CStrID>(CStrID("CameraPositionName"), CStrID::Empty);
	if (CameraPositionName.IsValid())
		CameraPosition = GlobalParams.GetConstant(CameraPositionName);

	OK;
}
//---------------------------------------------------------------------

}
