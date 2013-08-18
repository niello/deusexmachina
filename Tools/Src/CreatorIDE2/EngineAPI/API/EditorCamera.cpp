#include "../StdAPI.h"
#include <Game/EntityManager.h>
#include "../App/CIDEApp.h"
#include <Scene/Events/SetTransform.h>

API void EditorCamera_SetFocusEntity(App::CIDEAppHandle pHandle, const char* UID)
{
	DeclareCIDEApp(pHandle);

	Game::PEntity Ent = EntityMgr->GetEntity(CStrID(UID));

	const matrix44& TargetTfm = Ent->GetAttr<matrix44>(CStrID("Transform"));

	Event::SetTransform Evt(CIDEApp->EditorCamera->GetAttr<matrix44>(CStrID("Transform")));
	//!!!SetCOI & use distance that is stored IN CAMERA to keep it when changed by maya camera control!
	Evt.Transform.set_translation(TargetTfm.Translation() + Evt.Transform.AxisZ() * 20.f);
	CIDEApp->EditorCamera->FireEvent(Evt);
}
//---------------------------------------------------------------------
