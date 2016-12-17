#include "TargetEntity.h"

#include <Game/ActionContext.h>
#include <Game/Ability.h>
#include <Game/Action.h>
#include <Game/GameServer.h>
#include <AI/PropSmartObject.h> //!!!must be in Game, not AI!
#include <Data/Params.h>

namespace Game
{

CStrID CTargetEntity::GetTypeID() const
{
	static CStrID sidEntity("Entity");
	return sidEntity;
}
//---------------------------------------------------------------------

//???!!!reference actions by ID instead of pointers?! can then store centralized base of actions!
UPTR CTargetEntity::GetAvailableActions(const CActionContext& Context, CArray<IAction*>& OutActions) const
{
	n_assert(Context.pAbility);
	if (!Context.pAbility || !Context.pAbility->AcceptsTargetType(TypeFlag)) return 0;

	CProperty* pProp = GameSrv->GetEntityMgr()->GetProperty(EntityID, &Prop::CPropSmartObject::RTTI);
	if (pProp)
	{
		UPTR ActionsAdded = 0;
		//Get actions from prop
		//for (UPTR i = 0; i < action count; ++i)
		//{
		//	IAction* pAction = Actions[i];
		//	if (pAction->IsAvailable(Context, *this))
		//	{
		//		++ActionsAdded;
		//		OutActions.Add(pAction);
		//	}
		//}
	}

	IAction* pAction = Context.pAbility->GetAction();
	if (!pAction || !pAction->IsAvailable(Context, *this)) return 0;

	OutActions.Add(pAction);
	return 1;
}
//---------------------------------------------------------------------

void CTargetEntity::GetParams(Data::CParams& Out) const
{
	Out.Set(CStrID("Type"), GetTypeID());
	Out.Set(CStrID("EntityID"), EntityID);
}
//---------------------------------------------------------------------

}
