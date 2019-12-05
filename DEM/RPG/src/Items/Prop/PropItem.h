#pragma once
#include <Game/Property.h>
#include <Items/ItemStack.h>

// Item property contains item instance and allows to pick it up.

namespace Prop
{

class CPropItem: public Game::CProperty
{
	FACTORY_CLASS_DECL;
	__DeclarePropertyStorage;

protected:

	virtual bool InternalActivate();
	virtual void InternalDeactivate();

	DECLARE_EVENT_HANDLER(OnPropActivated, OnPropActivated);
	DECLARE_EVENT_HANDLER(OnLevelSaving, OnLevelSaving);

public:

	CPropItem();
	virtual ~CPropItem();

	Items::CItemStack Items;
};

}
