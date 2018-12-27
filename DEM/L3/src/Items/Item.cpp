#include "Item.h"

namespace Items
{

Ptr<CItem> CItem::Clone() const
{
#ifdef _DEBUG
	n_assert(sizeof(CItem) == sizeof(CItemTpl*)); // assert no other data stored not to remember swap string below!
#endif
	n_assert(Template/*.IsValid()*/);
	//???!!!CopyFields(); or virtual Clone is enough? call inherited.Clone() { copy filelds, return parent::Clone }?
	return const_cast<CItem*>(this); // Cause there are no per-instance fields we can avoid real cloning
	//return n_new(CItem)(Template); // Uncomment if assertion above failed (new fields added)
}
//---------------------------------------------------------------------

};
