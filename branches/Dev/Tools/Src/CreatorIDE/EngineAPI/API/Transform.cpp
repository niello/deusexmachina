#include <StdAPI.h>
#include <Game/Mgr/FocusManager.h>
#include <Game/Mgr/EntityManager.h>
#include <Game/Mgr/EnvQueryManager.h>
#include <Loading/EntityFactory.h>
#include <App/CIDEApp.h>
#include <Physics/Event/SetTransform.h>
#include <Input/Prop/PropInput.h>
#include <Physics/Prop/PropAbstractPhysics.h>
#include <Physics/CharEntity.h>
#include <Physics/Composite.h>

using namespace Properties;

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

