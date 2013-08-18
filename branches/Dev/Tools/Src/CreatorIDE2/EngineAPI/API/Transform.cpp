#include "../App/CIDEApp.h"
#include <Scene/Events/SetTransform.h>
#include <Game/GameServer.h>

API void Transform_SetGroundConstraints(App::CIDEAppHandle pHandle, bool NotAbove, bool NotBelow)
{
	DeclareCIDEApp(pHandle);

	CIDEApp->DenyEntityAboveGround = NotAbove;
	CIDEApp->DenyEntityBelowGround = NotBelow;
}
//---------------------------------------------------------------------

API void Transform_PlaceUnderMouse(App::CIDEAppHandle pHandle)
{
	DeclareCIDEApp(pHandle);

	if (!CIDEApp->SelectedEntity) return;

	Event::SetTransform Evt(CIDEApp->SelectedEntity->GetAttr<matrix44>(CStrID("Transform")));
	vector3 Pos = GameSrv->GetMousePos3D();
	CIDEApp->ApplyGroundConstraints(*CIDEApp->SelectedEntity, Pos);
	Evt.Transform.set_translation(Pos);
	CIDEApp->SelectedEntity->FireEvent(Evt);
}
//---------------------------------------------------------------------