#include "PropEditorCamera.h"

#include <Game/Mgr/FocusManager.h>
#include <Scene/SceneServer.h>
#include <Input/InputServer.h>
#include <Physics/Event/SetTransform.h>
#include <Physics/Prop/PropTransformable.h>

namespace Properties
{
ImplementRTTI(Properties::CPropEditorCamera, Properties::CPropCamera);
ImplementFactory(Properties::CPropEditorCamera);

void CPropEditorCamera::Activate()
{
	CPropCamera::Activate();
	SetupFromTransform();
	PROP_SUBSCRIBE_PEVENT(UpdateTransform, CPropEditorCamera, OnUpdateTransform);
}
//---------------------------------------------------------------------

void CPropEditorCamera::Deactivate()
{
	UNSUBSCRIBE_EVENT(UpdateTransform);
	CPropCamera::Deactivate();
}
//---------------------------------------------------------------------

void CPropEditorCamera::SetupFromTransform()
{
	const matrix44& Tfm = GetEntity()->Get<matrix44>(Attr::Transform);
	COI = Tfm.pos_component() - Tfm.z_component() * 10.0f;
	Distance = vector3::Distance(COI, Tfm.pos_component());
	vector3 ViewerDir = Tfm.pos_component() - COI;
	ViewerDir.norm();
	Angles.set(ViewerDir);
	Angles.theta -= N_PI * 0.5f;
	if (ViewerDir.y < 0) Angles.theta = -Angles.theta;
}
//---------------------------------------------------------------------

void CPropEditorCamera::OnObtainCameraFocus()
{
	CPropCamera::OnObtainCameraFocus();

	//???!!!set world transform?!
	SceneSrv->GetCurrentScene()->GetMainCamera()->GetNode()->SetLocalTransform(GetEntity()->Get<matrix44>(Attr::Transform));
	//AudioSrv->GetListener()->SetTransform(GfxSrv->GetCamera()->GetTransform());
}
//---------------------------------------------------------------------

// Subscribed only if camera has focus (see CPropCamera)
void CPropEditorCamera::OnRender()
{
	const matrix44& Tfm = GetEntity()->Get<matrix44>(Attr::Transform);

	matrix44 View;
	bool Changed = false;

	if (FocusMgr->GetInputFocusEntity() == GetEntity())
	{
		if (Distance < 0.f)
		{
			COI = Tfm.pos_component();
			Distance = 0.0f;
			Changed = true;
		}

		static const float CameraSens = 0.02f; //!!!TO SETTINGS!

		float MoveHoriz = -InputSrv->GetRawMouseMoveX() * CameraSens;
		float MoveVert = InputSrv->GetRawMouseMoveY() * CameraSens;

		if (MoveHoriz != 0.f || MoveVert != 0.f)
		{
			if (InputSrv->IsMouseBtnPressed(Input::MBLeft))
			{
				const float LookVel = 0.25f;
				Angles.theta -= MoveVert * LookVel;
				Angles.phi += MoveHoriz * LookVel;
				Changed = true;
			}

			if (InputSrv->IsMouseBtnPressed(Input::MBMiddle))
			{
				const float DefaultPanVel = 0.08f;
				const float MinPanVel = 0.08f;
				float PanVel = DefaultPanVel * Distance;
				if (PanVel < MinPanVel) PanVel = MinPanVel;
				COI += (Tfm.x_component() * MoveHoriz + Tfm.y_component() * MoveVert) * PanVel;
				Changed = true;
			}

			if (InputSrv->IsMouseBtnPressed(Input::MBRight))
			{
				const float DefaultZoomVel = 0.25f;
				const float MinZoomVel = 0.50f;
				float ZoomVel = DefaultZoomVel * Distance;
				if (ZoomVel < MinZoomVel) ZoomVel = MinZoomVel;
				Distance += (MoveHoriz - MoveVert) * ZoomVel;
				Changed = true;
			}
		}

		if (Changed)
		{
			View.translate(vector3(0.0f, 0.0f, Distance));
			View.rotate_x(Angles.theta);
			View.rotate_y(Angles.phi);
			View.translate(COI);
		}
	}

	if (Changed)
	{
		UpdateFromInside = true;
		GetEntity()->FireEvent(Event::SetTransform(View));
		UpdateFromInside = false;
	}
}
//---------------------------------------------------------------------

bool CPropEditorCamera::OnUpdateTransform(const Events::CEventBase& Event)
{
	if (!UpdateFromInside) SetupFromTransform();

	if (FocusMgr->GetCameraFocusEntity() == GetEntity())
	{
		//???!!!set world transform?!
		SceneSrv->GetCurrentScene()->GetMainCamera()->GetNode()->SetLocalTransform(GetEntity()->Get<matrix44>(Attr::Transform));
		//AudioSrv->GetListener()->SetTransform(GfxSrv->GetCamera()->GetTransform());
	}

	OK;
}
//---------------------------------------------------------------------

} // namespace Properties






