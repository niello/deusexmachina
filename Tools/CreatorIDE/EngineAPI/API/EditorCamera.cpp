#include <App/CIDEApp.h>
#include <Game/Mgr/EntityManager.h>
#include <Physics/Event/SetTransform.h>

namespace Attr
{
	DeclareAttr(Transform);
}

API void EditorCamera_SetFocusEntity(const char* UID)
{
	Game::PEntity Ent = EntityMgr->GetEntityByID(CStrID(UID));
	if (!Ent.isvalid()) return;

	const matrix44& TargetTfm = Ent->Get<matrix44>(Attr::Transform);

	Event::SetTransform Evt(CIDEApp->EditorCamera->Get<matrix44>(Attr::Transform));
	//!!!SetCOI & use distance that is stored IN CAMERA to keep it when changed by maya camera control!
	Evt.Transform.set_translation(TargetTfm.pos_component() + Evt.Transform.z_component() * 20.f);
	CIDEApp->EditorCamera->FireEvent(Evt);
}
//---------------------------------------------------------------------
