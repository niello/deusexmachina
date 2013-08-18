#include "PropEditorCamera.h"

#include <Game/GameServer.h>
#include <Game/Entity.h>
#include <Input/InputServer.h>
#include <Scene/Events/SetTransform.h>

namespace Scene
{
__ImplementClass(CPropEditorCamera, "ECAM", CCamera)

void CPropEditorCamera::Activate()
{
	CCamera::Activate(true);
	SetupFromTransform();
	/*PROP_SUBSCRIBE_PEVENT(UpdateTransform, CPropEditorCamera, OnUpdateTransform);*/
}
//---------------------------------------------------------------------

void CPropEditorCamera::Deactivate()
{
	/*UNSUBSCRIBE_EVENT(UpdateTransform);*/
	//CCamera::Deactivate();
}
//---------------------------------------------------------------------

void CPropEditorCamera::SetupFromTransform()
{
	/*const matrix44& Tfm = GetNode()->GetAttr<matrix44>(CStrID("Transform"));
	COI = Tfm.Translation() - Tfm.AxisZ() * 10.0f;
	Distance = vector3::Distance(COI, Tfm.Translation());
	vector3 ViewerDir = Tfm.Translation() - COI;
	ViewerDir.norm();
	Angles.Set(ViewerDir);
	Angles.Theta -= PI * 0.5f;
	if (ViewerDir.y < 0) Angles.Theta = -Angles.Theta;*/
}
//---------------------------------------------------------------------

void CPropEditorCamera::OnObtainCameraFocus()
{
	/*CCamera::OnObtainCameraFocus();
	GfxSrv->GetCamera()->SetTransform(GetEntity()->Get<matrix44>(Attr::Transform));*/
	//AudioSrv->GetListener()->SetTransform(GfxSrv->GetCamera()->GetTransform());
}
//---------------------------------------------------------------------

// Subscribed only if camera has focus (see CPropCamera)
void CPropEditorCamera::OnRender()
{
	//const matrix44& Tfm = GetNode()->GetAttr<matrix44>(CStrID("Transform"));

	//matrix44 View;
	//bool Changed = false;

	//if (GameSrv->GetEntityUnderMouse()->GetUID() == GetNode()->GetName())
	//{
	//	if (Distance < 0.f)
	//	{
	//		COI = Tfm.pos_component();
	//		Distance = 0.0f;
	//		Changed = true;
	//	}

	//	static const float CameraSens = 0.02f; //!!!TO SETTINGS!

	//	float MoveHoriz = -InputSrv->GetRawMouseMoveX() * CameraSens;
	//	float MoveVert = InputSrv->GetRawMouseMoveY() * CameraSens;

	//	if (MoveHoriz != 0.f || MoveVert != 0.f)
	//	{
	//		if (InputSrv->IsMouseBtnPressed(Input::MBLeft))
	//		{
	//			const float LookVel = 0.25f;
	//			Angles.Theta -= MoveVert * LookVel;
	//			Angles.Phi += MoveHoriz * LookVel;
	//			Changed = true;
	//		}

	//		if (InputSrv->IsMouseBtnPressed(Input::MBMiddle))
	//		{
	//			const float DefaultPanVel = 0.08f;
	//			const float MinPanVel = 0.08f;
	//			float PanVel = DefaultPanVel * Distance;
	//			if (PanVel < MinPanVel) PanVel = MinPanVel;
	//			COI += (Tfm.x_component() * MoveHoriz + Tfm.y_component() * MoveVert) * PanVel;
	//			Changed = true;
	//		}

	//		if (InputSrv->IsMouseBtnPressed(Input::MBRight))
	//		{
	//			const float DefaultZoomVel = 0.25f;
	//			const float MinZoomVel = 0.50f;
	//			float ZoomVel = DefaultZoomVel * Distance;
	//			if (ZoomVel < MinZoomVel) ZoomVel = MinZoomVel;
	//			Distance += (MoveHoriz - MoveVert) * ZoomVel;
	//			Changed = true;
	//		}
	//	}

	//	if (Changed)
	//	{
	//		View.translate(vector3(0.0f, 0.0f, Distance));
	//		View.rotate_x(Angles.Theta);
	//		View.rotate_y(Angles.Phi);
	//		View.translate(COI);
	//	}
	//}

	//if (Changed)
	//{
	//	UpdateFromInside = true;
	//	GameSrv->GetEntityUnderMouse()->FireEvent(Event::SetTransform(View));
	//	UpdateFromInside = false;
	//}
}
//---------------------------------------------------------------------

bool CPropEditorCamera::OnUpdateTransform(const Events::CEventBase& Event)
{
	if (!UpdateFromInside) SetupFromTransform();

	//if (GameSrv->GetEntityUnderMouse()->GetUID() == GetNode()->GetName())
	//{
	//	GfxSrv->GetCamera()->SetTransform(GetEntity()->Get<matrix44>(Attr::Transform));
	//	//AudioSrv->GetListener()->SetTransform(GfxSrv->GetCamera()->GetTransform());
	//}

	OK;
}
//---------------------------------------------------------------------

} // namespace Properties






