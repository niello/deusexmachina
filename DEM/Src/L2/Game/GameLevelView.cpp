#include "GameLevelView.h"

#include <Game/GameLevel.h>
#include <Frame/NodeAttrCamera.h>

namespace Game
{

bool CGameLevelView::Setup(CGameLevel& GameLevel, HHandle hView)
{
	Level = &GameLevel;
	View.pSPS = GameLevel.GetSPS();
	//???fill other fields?
	OK;
}
//---------------------------------------------------------------------

const vector3& CGameLevelView::GetCenterOfInterest() const
{
	return View.GetCamera()->GetNode()->GetWorldPosition();
}
//---------------------------------------------------------------------

void CGameLevelView::AddToSelection(CStrID EntityID)
{
	if (IsSelected(EntityID) || !EntityID.IsValid() || !Level->HostsEntity(EntityID)) return;
	SelectedEntities.Add(EntityID);
	Data::PParams P = n_new(Data::CParams(1));
	P->Set(CStrID("EntityID"), EntityID);
	//???add view handle as int? or as UPTR/IPTR?
	Level->FireEvent(CStrID("OnEntitySelected"), P);
}
//---------------------------------------------------------------------

bool CGameLevelView::RemoveFromSelection(CStrID EntityID)
{
	if (!SelectedEntities.RemoveByValue(EntityID)) FAIL; 
	Data::PParams P = n_new(Data::CParams(1));
	P->Set(CStrID("EntityID"), EntityID);
	Level->FireEvent(CStrID("OnEntityDeselected"), P);
	OK;
}
//---------------------------------------------------------------------

}