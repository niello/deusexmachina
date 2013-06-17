#include "ItemManager.h"

#include <IO/IOServer.h>
#include <Data/Params.h>
#include <Data/DataServer.h>

namespace Items
{
__ImplementClassNoFactory(Items::CItemManager, Core::CRefCounted);
__ImplementSingleton(Items::CItemManager);

CItemManager::CItemManager()
{
	__ConstructSingleton;
	IOSrv->SetAssign("items", "game:items");
}
//---------------------------------------------------------------------

PItemTpl CItemManager::CreateItemTpl(CStrID ID, const CParams& Params)
{
	PItemTpl Tpl = (CItemTpl*)Factory->Create(nString("Items::CItemTpl") + Params.Get<nString>(CStrID("Type"), NULL));
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
		PParams HRD = DataSrv->LoadPRM(nString("items:") + ID.CStr() + ".prm", false);
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