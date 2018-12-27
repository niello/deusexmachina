#pragma once
#ifndef __IPG_PROP_DESTRUCTIBLE_H__
#define __IPG_PROP_DESTRUCTIBLE_H__

#include <Game/Property.h>

// Destructible property allows to logically destroy owning entity by applying damage to it.

namespace Prop
{

using namespace Events;

class CPropDestructible: public Game::CProperty
{
	__DeclareClass(CPropDestructible);
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

#endif