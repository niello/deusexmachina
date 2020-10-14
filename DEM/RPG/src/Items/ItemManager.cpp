#include "ItemManager.h"

#include <Data/Params.h>
#include <Data/ParamsUtils.h>
#include <Core/Factory.h>

namespace Items
{
__ImplementSingleton(Items::CItemManager);

PItemTpl CItemManager::CreateItemTpl(CStrID ID, const Data::CParams& Params)
{
	PItemTpl Tpl = Core::CFactory::Instance().Create<CItemTpl>(CString("Items::CItemTpl") + Params.Get<CString>(CStrID("Type"), CString::Empty));
	n_assert(Tpl.IsValidPtr());
	Tpl->Init(ID, Params);
	return Tpl;
}
//---------------------------------------------------------------------

PItemTpl CItemManager::GetItemTpl(CStrID ID)
{
	PItemTpl Tpl;
	if (ItemTplRegistry.Get(ID, Tpl)) return Tpl;
	else
	{
		Data::PParams HRD = ParamsUtils::LoadParamsFromPRM(CString("Items:") + ID.CStr() + ".prm");
		if (!HRD) return PItemTpl();

		Tpl = CreateItemTpl(ID, *HRD);
		n_assert(Tpl.IsValidPtr());
		ItemTplRegistry.Add(ID, Tpl);
		return Tpl;
	}
}
//---------------------------------------------------------------------

}
