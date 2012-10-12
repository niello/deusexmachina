#include <App/CIDEApp.h>
#include <Game/Mgr/EnvQueryManager.h>
#include <Physics/Event/SetTransform.h>

namespace Attr
{
	DeclareAttr(Transform);
}

API void Transform_SetGroundConstraints(bool NotAbove, bool NotBelow)
{
	CIDEApp->DenyEntityAboveGround = NotAbove;
	CIDEApp->DenyEntityBelowGround = NotBelow;
}
//---------------------------------------------------------------------

API void Transform_PlaceUnderMouse()
{
	if (!CIDEApp->SelectedEntity.isvalid()) return;

	Event::SetTransform Evt(CIDEApp->SelectedEntity->Get<matrix44>(Attr::Transform));
	vector3 Pos = EnvQueryMgr->GetMousePos3D();
	CIDEApp->ApplyGroundConstraints(*CIDEApp->SelectedEntity, Pos);
	Evt.Transform.set_translation(Pos);
	CIDEApp->SelectedEntity->FireEvent(Evt);
}
//---------------------------------------------------------------------

