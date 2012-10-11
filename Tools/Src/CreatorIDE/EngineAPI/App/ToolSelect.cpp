#include "ToolSelect.h"

#include <Events/EventManager.h>
#include <Input/InputServer.h>
#include <Input/Events/MouseBtnDown.h>
#include <Game/Mgr/EnvQueryManager.h>
#include <App/CIDEApp.h>

namespace App
{
//ImplementRTTI(App::CToolSelect, App::IEditorTool);

void CToolSelect::Activate()
{
	SUBSCRIBE_INPUT_EVENT(MouseBtnDown, CToolSelect, OnMouseBtnDown, Input::InputPriority_Mapping);
}
//---------------------------------------------------------------------

void CToolSelect::Deactivate()
{
	UNSUBSCRIBE_EVENT(MouseBtnDown);
}
//---------------------------------------------------------------------

bool CToolSelect::OnMouseBtnDown(const Events::CEventBase& Event)
{
	const Event::MouseBtnDown& Ev = ((const Event::MouseBtnDown&)Event);
	if (Ev.Button != Input::MBLeft) FAIL;

	Game::CEntity* pEnt = EnvQueryMgr->GetEntityUnderMouse();
	if (!pEnt) FAIL;
	CIDEApp->SelectedEntity = pEnt;

	PParams P = n_new(CParams(1));
	P->Set(CStrID("UID"), pEnt->GetUniqueID());
	EventMgr->FireEvent(CStrID("OnEntitySelected"), P);

	OK;
}
//---------------------------------------------------------------------

}