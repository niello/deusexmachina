#include "PropVideoCamera2.h"

#include <Game/Mgr/FocusManager.h>
#include <Gfx/GfxServer.h>
#include <Gfx/CameraEntity.h>
#include <Physics/Prop/PropTransformable.h> //!!!for Attr::Transform
#include <Physics/Prop/PropPathAnim.h>
#include <DB/DBServer.h>
#include <Loading/EntityFactory.h>

namespace Attr
{
	DefineFloat(FilmAspectRatio);
	DefineFloat(NearClipPlane);
	DefineFloat(FarClipPlane);
	DefineMatrix44(ProjectionMatrix);
};

BEGIN_ATTRS_REGISTRATION(PropVideoCamera2)
	RegisterFloatWithDefault(FilmAspectRatio, ReadOnly, 1.33f);
	RegisterFloatWithDefault(NearClipPlane, ReadOnly, 0.1f);
	RegisterFloatWithDefault(FarClipPlane, ReadOnly, 1000.f);
	RegisterMatrix44(ProjectionMatrix, ReadOnly);
END_ATTRS_REGISTRATION

namespace Properties
{
ImplementRTTI(Properties::CPropVideoCamera2, Properties::CPropCamera);
ImplementFactory(Properties::CPropVideoCamera2);
RegisterProperty(CPropVideoCamera2);

void CPropVideoCamera2::Activate()
{
	CPropCamera::Activate();

	const matrix44& m = GetEntity()->Get<matrix44>(Attr::Transform);
	GfxSrv->GetCamera()->SetTransform(m); //???on obtain focus?
	//MayaCamera.SetDefaultEyePos(m.pos_component());
	//MayaCamera.SetDefaultUpVec(vector3(0.0f, 1.0f, 0.0f));
	//MayaCamera.SetDefaultCenterOfInterest(m.pos_component() - m.z_component() * 10.0f);
	//MayaCamera.Initialize();

	PROP_SUBSCRIBE_PEVENT(UpdateTransform, CPropVideoCamera2, OnUpdateTransform);
}
//---------------------------------------------------------------------

void CPropVideoCamera2::Deactivate()
{
	UNSUBSCRIBE_EVENT(UpdateTransform);
	CPropCamera::Deactivate();
}
//---------------------------------------------------------------------

void CPropVideoCamera2::GetAttributes(nArray<DB::CAttrID>& Attrs)
{
	CPropCamera::GetAttributes(Attrs);
	Attrs.Append(Attr::FilmAspectRatio);
	Attrs.Append(Attr::NearClipPlane);
	Attrs.Append(Attr::FarClipPlane);

	//GetEntity()->Set<float>(Attr::FilmAspectRatio, 1.33f);
	//GetEntity()->Set<float>(Attr::NearClipPlane, 0.1f);
	//GetEntity()->Set<float>(Attr::FarClipPlane, 1000.0f);
}
//---------------------------------------------------------------------

void CPropVideoCamera2::OnObtainCameraFocus()
{
	Graphics::CCameraEntity* pCamera = GfxSrv->GetCamera();
	pCamera->GetCamera().SetAngleOfView(GetEntity()->Get<float>(Attr::FieldOfView));
	pCamera->GetCamera().SetAspectRatio(GetEntity()->Get<float>(Attr::FilmAspectRatio));
	pCamera->GetCamera().SetNearPlane(GetEntity()->Get<float>(Attr::NearClipPlane));
	pCamera->GetCamera().SetFarPlane(GetEntity()->Get<float>(Attr::FarClipPlane));
	pCamera->GetCamera().SetProjectionMatrix(GetEntity()->Get<matrix44>(Attr::ProjectionMatrix));
}
//---------------------------------------------------------------------

void CPropVideoCamera2::OnRender()
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
		// only use the internal matrix if we are not animated
		//if (!(GetEntity()->HasAttr(Attr::AnimPath) && GetEntity()->Get<nString>(Attr::AnimPath).IsValid()))
		//	GfxSrv->GetCamera()->SetTransform(MayaCamera.GetViewMatrix());
	}

	// important: call parent class at the end to apply any shake effects
	CPropCamera::OnRender();
}
//---------------------------------------------------------------------

bool CPropVideoCamera2::OnUpdateTransform(const CEventBase& Event)
{
	// this is coming usually from the AnimPath property
	//???cache Attr::Transform matrix ref-ptr?
	GfxSrv->GetCamera()->SetTransform(GetEntity()->Get<matrix44>(Attr::Transform));
	OK;
}
//---------------------------------------------------------------------

} // namespace Properties
