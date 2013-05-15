#include "ItemTpl.h"

#include "Item.h"
#include <Data/Params.h>
#include <Data/DataArray.h>

namespace Items
{
//__ImplementClassNoFactory(Items::CItemTpl, Core::CRefCounted);
__ImplementClass(Items::CItemTpl);

void CItemTpl::Init(CStrID SID, const CParams& Params)
{
	ID = SID;
	Type = CStrID(Params.Get<nString>(CStrID("Type"), NULL).CStr());
	Weight = Params.Get<float>(CStrID("Weight"), 0.f);
	Volume = Params.Get<float>(CStrID("Volume"), 0.f);
	UIName = Params.Get<nString>(CStrID("UIName"), NULL);

	// Can be created by derived type if CItem's derived type needed
	if (!TemplateItem.IsValid()) TemplateItem = n_new(CItem)(this);

	// Init CItem params here
}
//---------------------------------------------------------------------

Ptr<CItem> CItemTpl::CreateNewItem() const
{
	n_assert(TemplateItem.IsValid());
	return TemplateItem->Clone();
}
//---------------------------------------------------------------------

};
