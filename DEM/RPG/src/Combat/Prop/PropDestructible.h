#pragma once
#include <Game/Property.h>

// Destructible property allows to logically destroy owning entity by applying damage to it.

namespace Prop
{

using namespace Events;

class CPropDestructible: public Game::CProperty
{
	FACTORY_CLASS_DECL;
	__DeclarePropertyStorage;

protected:

	//!!!resists!

	virtual bool InternalActivate();
	virtual void InternalDeactivate();

	DECLARE_EVENT_HANDLER(ObjDamageDone, OnObjDamageDone);

public:

	//!!!to attributes!
	int	HP, HPMax;

	//CPropDestructible();
	//virtual ~CPropDestructible();
};

}
