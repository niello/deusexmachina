#include "PropSimpleCamera.h"

#include <Game/Mgr/FocusManager.h>
#include <Game/Entity.h>
#include <Gfx/GfxServer.h>
#include <Gfx/CameraEntity.h>
#include <Audio/AudioServer.h>
#include <Camera/Event/CameraOrbit.h>
#include <Events/Event.h>

namespace Properties
{
ImplementRTTI(Properties::CPropSimpleCamera, Properties::CPropCamera);
ImplementFactory(Properties::CPropSimpleCamera);

using namespace Game;

CPropSimpleCamera::CPropSimpleCamera():
	RelHorizontalRotation(0.f),
	RelHorizontalRotationFactor(0.15f),
	RelVerticalRotation(0.f),
	RelVerticalRotationFactor(0.15f),
	RelMove(0.f, 0.f, 0.f),
	RelMoveFactor(20.f)
{
}
//---------------------------------------------------------------------

void CPropSimpleCamera::Activate()
{
	CPropCamera::Activate();

	PROP_SUBSCRIBE_PEVENT(MoveByOffset, CPropSimpleCamera, OnMoveByOffset);
	PROP_SUBSCRIBE_PEVENT(CameraOrbit, CPropSimpleCamera, OnCameraOrbit);
}
//---------------------------------------------------------------------

void CPropSimpleCamera::Deactivate()
{
	UNSUBSCRIBE_EVENT(MoveByOffset);
	UNSUBSCRIBE_EVENT(CameraOrbit);

	CPropCamera::Deactivate();
}
//---------------------------------------------------------------------

void CPropSimpleCamera::OnRender()
{
	if (!HasFocus()) return;

	Graphics::CCameraEntity* CamEntity = GfxSrv->GetCamera();
	n_assert(CamEntity != 0);

	matrix44 NewTransform = CamEntity->GetTransform();

	vector3 curPosition = NewTransform.pos_component();

	NewTransform.set_translation(vector3(0.f, 0.f, 0.f));
	NewTransform.rotate(vector3(0.f, 1.f, 0.f), -RelHorizontalRotation * RelHorizontalRotationFactor);
	vector3 xRotAxis(1.f, 0.f, 0.f);
	xRotAxis = NewTransform * xRotAxis;
	xRotAxis.y = 0;
	xRotAxis.norm();
	NewTransform.rotate(xRotAxis, RelVerticalRotation * -RelVerticalRotationFactor);

	NewTransform.set_translation(curPosition);

	NewTransform.translate(NewTransform.z_component() * RelMove.z * RelMoveFactor);
	NewTransform.translate(NewTransform.x_component() * RelMove.x * RelMoveFactor);

	RelHorizontalRotation = 0.f;
	RelVerticalRotation = 0.f;
	RelMove = vector3(0.f, 0.f, 0.f);

	CamEntity->SetTransform(NewTransform);

	AudioSrv->ListenerTransform = CamEntity->GetTransform();
}
//---------------------------------------------------------------------

bool CPropSimpleCamera::OnMoveByOffset(const Events::CEventBase& Event)
{
	RelMove += ((const Events::CEvent&)Event).Params->Get<vector3>(CStrID("Offset"));
	OK;
}
//---------------------------------------------------------------------

bool CPropSimpleCamera::OnCameraOrbit(const Events::CEventBase& Event)
{
	RelHorizontalRotation += ((const Event::CameraOrbit&)Event).AngleHoriz;
	RelVerticalRotation += ((const Event::CameraOrbit&)Event).AngleVert;
	OK;
}
//---------------------------------------------------------------------

} // namespace Properties
