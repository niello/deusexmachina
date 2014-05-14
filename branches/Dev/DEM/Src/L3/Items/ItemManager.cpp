#include "ItemManager.h"

#include <Data/Params.h>
#include <Data/DataServer.h>
#include <Core/Factory.h>

namespace Items
{
__ImplementClassNoFactory(Items::CItemManager, Core::CObject);
__ImplementSingleton(Items::CItemManager);

PItemTpl CItemManager::CreateItemTpl(CStrID ID, const Data::CParams& Params)
{
	PItemTpl Tpl = (CItemTpl*)Factory->Create(CString("Items::CItemTpl") + Params.Get<CString>(CStrID("Type"), NULL));
	n_assert(Tpl.IsValid());
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
		Data::PParams HRD = DataSrv->LoadPRM(CString("Items:") + ID.CStr() + ".prm", false);
		if (HRD.IsValid())
		{
			Tpl = CreateItemTpl(ID, *HRD);
			n_assert(Tpl.IsValid());
			ItemTplRegistry.Add(ID, Tpl);
			return Tpl;
		}
		else return PItemTpl();
	}
}
//---------------------------------------------------------------------

}