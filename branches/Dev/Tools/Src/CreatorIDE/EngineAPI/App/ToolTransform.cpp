#include "ToolTransform.h"

#include <App/CIDEApp.h>
#include <Events/EventManager.h>
#include <Physics/Event/SetTransform.h>
#include <Physics/Prop/PropAbstractPhysics.h>
#include <Input/InputServer.h>
#include <Render/RenderServer.h>
#include <Render/DebugDraw.h>
#include <Scene/SceneServer.h>
#include <Game/Mgr/FocusManager.h>

namespace Attr
{
	DeclareAttr(Transform);
}

namespace App
{
//ImplementRTTI(App::CToolSelect, App::IEditorTool);

void CToolTransform::Activate()
{
	FocusMgr->SetInputFocusEntity(NULL);
	SUBSCRIBE_PEVENT(OnBeginFrame, CToolTransform, OnBeginFrame);
}
//---------------------------------------------------------------------

void CToolTransform::Deactivate()
{
	UNSUBSCRIBE_EVENT(OnBeginFrame);
	FocusMgr->SetInputFocusEntity(CIDEApp->EditorCamera);
}
//---------------------------------------------------------------------

bool CToolTransform::OnBeginFrame(const Events::CEventBase& Event)
{
	if (!CIDEApp->SelectedEntity.isvalid()) OK;

	static const float MoveVelocityXY = 0.33f;
	static const float MoveVelocityZ = 0.33f;
	static const float RotateVelocityXY = 4.f;
	static const float RotateVelocityZ = 1.f;

	bool TfmChanged = false;

	Event::SetTransform Evt(CIDEApp->SelectedEntity->Get<matrix44>(Attr::Transform));

	vector3 Pos = Evt.Transform.pos_component();
	
	float MoveX, MoveY;
	float MoveZ = (float)InputSrv->GetWheelTotal() * -0.3f; //???make distance-dependent?

	//???!!!use raw or cursor?!
	//???may be reproject ray and intersect with plane instead of adjusting drag speed?
	RenderSrv->GetDisplay().GetRelativeXY(InputSrv->GetRawMouseMoveX(), -InputSrv->GetRawMouseMoveY(), MoveX, MoveY);

	Scene::CCamera& Camera = *SceneSrv->GetCurrentScene()->GetMainCamera();
	const matrix44& View = Camera.GetInvViewMatrix(); //???inv?
	
	vector3 Offset = View.pos_component() - Pos;

	if (InputSrv->IsMouseBtnPressed(Input::MBLeft) && (MoveX || MoveY))
	{
		//???why MoveVelocityXY != 1.f? error in calculations?
		const vector3& Axis = View.z_component();
		vector3 ProjectedOffset = Axis * (Offset % Axis);
		float DistanceToPlane = ProjectedOffset.len();
		float FOV = Camera.GetFOV();
		float SizeY = 2 * DistanceToPlane * tanf(FOV);
		TfmChanged = true;
		if (MoveX)
		{
			float SizeX = SizeY * Camera.GetAspectRatio();
			Pos += View.x_component() * (SizeX * MoveX * MoveVelocityXY);
		}
		if (MoveY) Pos += View.y_component() * (SizeY * MoveY * MoveVelocityXY);
	}
	
	if (InputSrv->IsMouseBtnPressed(Input::MBRight))
	{
		Evt.Transform.set_translation(vector3(0.f, 0.f, 0.f));
		if (MoveX)
		{
			Evt.Transform.rotate(View.y_component(), -MoveX * RotateVelocityXY);
			TfmChanged = true;
		}
		if (MoveY)
		{
			Evt.Transform.rotate(View.x_component(), MoveY * RotateVelocityXY);
			TfmChanged = true;
		}
		if (MoveZ)
		{
			Evt.Transform.rotate(View.z_component(), MoveZ * RotateVelocityZ);
			TfmChanged = true;
		}
	}
	else if (MoveZ)
	{
		Pos += View.z_component() * (MoveZ * MoveVelocityZ * Offset.len());
		TfmChanged = true;
	}

	if (TfmChanged)
	{
		CIDEApp->ApplyGroundConstraints(*CIDEApp->SelectedEntity, Pos);
		Evt.Transform.set_translation(Pos);
		CIDEApp->SelectedEntity->FireEvent(Evt);
	}

	OK;
}
//---------------------------------------------------------------------

void CToolTransform::Render()
{
	if (CIDEApp->SelectedEntity.isvalid())
	{
		DebugDraw->DrawCoordAxes(CIDEApp->SelectedEntity->Get<matrix44>(Attr::Transform));

		Properties::CPropAbstractPhysics* pPhysProp = CIDEApp->SelectedEntity->FindProperty<Properties::CPropAbstractPhysics>();
		if (pPhysProp)
		{
			bbox3 AABB;
			pPhysProp->GetAABB(AABB);
			DebugDraw->DrawBoxWireframe(AABB, vector4::White);
		}
	}
}
//---------------------------------------------------------------------

}