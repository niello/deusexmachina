#pragma once
#ifndef __IPG_PROP_DESTRUCTIBLE_H__
#define __IPG_PROP_DESTRUCTIBLE_H__

#include <game/property.h>

// Destructible property allows to logically destroy owning entity by applying damage to it.

// Adds IAO actions:
// - Attack

namespace Prop
{

using namespace Events;

class CPropDestructible: public Game::CProperty
{
	__DeclareClass(CPropDestructible);
	__DeclarePropertyStorage;

protected:

	//!!!resists!

	DECLARE_EVENT_HANDLER(ObjDamageDone, OnObjDamageDone);

public:

	//!!!to attributes!
	int	HP, HPMax;

	//CPropDestructible();
	//virtual ~CPropDestructible();

	virtual void	Activate();
	virtual void	Deactivate();
};

}

#endif