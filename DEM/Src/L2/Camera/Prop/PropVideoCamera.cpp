#include "PropVideoCamera.h"

#include <Game/Mgr/FocusManager.h>
#include <Game/Entity.h>
#include <Gfx/GfxServer.h>
#include <Gfx/CameraEntity.h>
#include <Physics/Prop/PropPathAnim.h>
#include <Physics/Prop/PropTransformable.h>
#include <DB/DBServer.h>
#include <Loading/EntityFactory.h>

namespace Attr
{
	DefineVector3(VideoCameraCenterOfInterest);
	DefineVector3(VideoCameraDefaultUpVec);
};

BEGIN_ATTRS_REGISTRATION(PropVideoCamera)
	RegisterVector3(VideoCameraCenterOfInterest, ReadOnly);
	RegisterVector3WithDefault(VideoCameraDefaultUpVec, ReadOnly, vector4(0.0f, 1.0f, 0.0f, 0.f));
END_ATTRS_REGISTRATION

namespace Properties
{
ImplementRTTI(Properties::CPropVideoCamera, Properties::CPropCamera);
ImplementFactory(Properties::CPropVideoCamera);

void CPropVideoCamera::Activate()
{
	CPropCamera::Activate();

	//const matrix44& m = GetEntity()->Get<matrix44>(Attr::Transform);
	//MayaCamera.SetDefaultEyePos(m.pos_component());
	////MayaCamera.SetDefaultUpVec(m.y_component());
	////MayaCamera.SetDefaultCenterOfInterest(m.pos_component() - m.z_component() * 10.0f);
	//vector3 V;
	//GetEntity()->Get<vector3>(Attr::VideoCameraCenterOfInterest, V);
	//MayaCamera.SetDefaultCenterOfInterest(V);
	//GetEntity()->Get<vector3>(Attr::VideoCameraDefaultUpVec, V);
	//MayaCamera.SetDefaultUpVec(V);
	//MayaCamera.Initialize();

	PROP_SUBSCRIBE_PEVENT(UpdateTransform, CPropVideoCamera, OnUpdateTransform);
}
//---------------------------------------------------------------------

void CPropVideoCamera::Deactivate()
{
	UNSUBSCRIBE_EVENT(UpdateTransform);
	CPropCamera::Deactivate();
}
//---------------------------------------------------------------------

void CPropVideoCamera::GetAttributes(nArray<DB::CAttrID>& Attrs)
{
	CPropCamera::GetAttributes(Attrs);
	Attrs.Append(Attr::VideoCameraCenterOfInterest);
	Attrs.Append(Attr::VideoCameraDefaultUpVec);

	//GetEntity()->Set<vector4>(Attr::VideoCameraCenterOfInterest, vector4());
	//GetEntity()->Set<vector4>(Attr::VideoCameraDefaultUpVec, vector4(0.0f, 1.0f, 0.0f, 0.f));
}
//---------------------------------------------------------------------

void CPropVideoCamera::OnRender()
{
	if (FocusMgr->GetInputFocusEntity() == GetEntity())
	{
		//if (nInputServer::Instance()->GetButton("ctrlPressed"))
		//{
		//	MayaCamera.SetResetButton(nInputServer::Instance()->GetButton("vwrReset"));
		//	MayaCamera.SetLookButton(nInputServer::Instance()->GetButton("lmbPressed"));
		//	MayaCamera.SetPanButton(nInputServer::Instance()->GetButton("mmbPressed"));
		//	MayaCamera.SetZoomButton(nInputServer::Instance()->GetButton("rmbPressed"));
		//	MayaCamera.SetSliderLeft(nInputServer::Instance()->GetSlider("vwrLeft"));
		//	MayaCamera.SetSliderRight(nInputServer::Instance()->GetSlider("vwrRight"));
		//	MayaCamera.SetSliderUp(nInputServer::Instance()->GetSlider("vwrUp"));
		//	MayaCamera.SetSliderDown(nInputServer::Instance()->GetSlider("vwrDown"));
		//}

		//MayaCamera.Update();
	}

	if (HasFocus())
	{
		if (!(GetEntity()->HasAttr(Attr::AnimPath) && GetEntity()->Get<nString>(Attr::AnimPath).IsValid()))
		{
			GfxSrv->GetCamera()->GetCamera().SetPerspective(GetEntity()->Get<float>(Attr::FieldOfView),
												4.0f/3.0f, 0.1f, 2500.0f);
			//GfxSrv->GetCamera()->SetTransform(MayaCamera.GetViewMatrix());
		}
	}

	// important: call parent class at the end to apply any shake effects
	CPropCamera::OnRender();
}
//---------------------------------------------------------------------

bool CPropVideoCamera::OnUpdateTransform(const CEventBase& Event)
{
	// this is coming usually from the AnimPath property
	//???cache Attr::Transform matrix ref-ptr?
	GfxSrv->GetCamera()->SetTransform(GetEntity()->Get<matrix44>(Attr::Transform));
	OK;
}
//---------------------------------------------------------------------

} // namespace Properties






