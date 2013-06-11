#include "PropDestructible.h"

#include <Combat/Event/ObjDamageDone.h>

namespace Prop
{
__ImplementClass(Prop::CPropDestructible, 'PDST', Game::CProperty);
__ImplementPropertyStorage(CPropDestructible);

using namespace Event;

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

bool CPropDestructible::OnObjDamageDone(const CEventBase& Event)
{
	const ObjDamageDone& e = (const ObjDamageDone&)Event;

	HP -= e.Amount;
	n_printf("CEntity \"%s\": Fucking shit! I was damaged. HP:%d/%d\n",
		GetEntity()->GetUID(),
		HP,
		HPMax);

	if (HP <= 0)
	{
		//!!!send ObjDie/ObjDestructed msg!
		//???notify killer entity about success to gain exp?
		n_printf("CEntity \"%s\": I'm dead!\n", GetEntity()->GetUID());
	}

	OK;
}
//---------------------------------------------------------------------

} // namespace Prop
