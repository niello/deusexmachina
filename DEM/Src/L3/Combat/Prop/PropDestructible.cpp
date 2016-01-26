#include "PropDestructible.h"

#include <Combat/Event/ObjDamageDone.h>
#include <Game/Entity.h>
#include <Core/Factory.h>

namespace Prop
{
__ImplementClass(Prop::CPropDestructible, 'PDST', Game::CProperty);
__ImplementPropertyStorage(CPropDestructible);

bool CPropDestructible::InternalActivate()
{
	//!!!!!DBG!
	HP = HPMax = 12;
	PROP_SUBSCRIBE_NEVENT(ObjDamageDone, CPropDestructible, OnObjDamageDone);

	OK;
}
//---------------------------------------------------------------------

void CPropDestructible::InternalDeactivate()
{
	UNSUBSCRIBE_EVENT(ObjDamageDone);
}
//---------------------------------------------------------------------

bool CPropDestructible::OnObjDamageDone(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	const Event::ObjDamageDone& e = (const Event::ObjDamageDone&)Event;

	HP -= e.Amount;
	Sys::Log("CEntity \"%s\": Fucking shit! I was damaged. HP:%d/%d\n",
		GetEntity()->GetUID(),
		HP,
		HPMax);

	if (HP <= 0)
	{
		//!!!send ObjDie/ObjDestructed msg!
		//???notify killer entity about success to gain exp?
		Sys::Log("CEntity \"%s\": I'm dead!\n", GetEntity()->GetUID());
	}

	OK;
}
//---------------------------------------------------------------------

} // namespace Prop
