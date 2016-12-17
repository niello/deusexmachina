#include "TargetGround.h"

#include <Game/ActionContext.h>
#include <Game/Ability.h>
#include <Game/Action.h>
//#include <Data/Params.h>

namespace Game
{

CStrID CTargetGround::GetTypeID() const
{
	static CStrID sidGround("Ground");
	return sidGround;
}
//---------------------------------------------------------------------

//???!!!reference actions by ID instead of pointers?! can then store centralized base of actions!
UPTR CTargetGround::GetAvailableActions(const CActionContext& Context, CArray<IAction*>& OutActions) const
{
	n_assert(Context.pAbility);
	if (!Context.pAbility || !Context.pAbility->AcceptsTargetType(TypeFlag)) return 0;

	IAction* pAction = Context.pAbility->GetAction();
	if (!pAction || !pAction->IsAvailable(Context, *this)) return 0;

	OutActions.Add(pAction);
	return 1;
}
//---------------------------------------------------------------------

void CTargetGround::GetParams(Data::CParams& Out) const
{
	Out.Set(CStrID("Type"), GetTypeID());
	Out.Set(CStrID("Position"), Position);
}
//---------------------------------------------------------------------

}
