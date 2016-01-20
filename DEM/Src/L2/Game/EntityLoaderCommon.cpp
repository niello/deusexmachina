#include "EntityLoaderCommon.h"

#include <Game/EntityManager.h>
#include <Game/GameLevel.h>
#include <Data/DataArray.h>

namespace Game
{
__ImplementClass(Game::CEntityLoaderCommon, 'ELCM', Game::IEntityLoader);

bool CEntityLoaderCommon::Load(CStrID UID, CGameLevel& Level, const Data::CParams& Desc)
{
	n_assert(UID.IsValid());
	PEntity Entity = EntityMgr->CreateEntity(UID, Level);
	if (Entity.IsNullPtr()) FAIL;

	Data::PParams SubDesc;
	if (Desc.Get(SubDesc, CStrID("Attrs")) && SubDesc->GetCount())
	{
		Entity->BeginNewAttrs(SubDesc->GetCount());
		for (UPTR i = 0; i < SubDesc->GetCount(); ++i)
		{
			const Data::CParam& Attr = SubDesc->Get(i);
			Entity->AddNewAttr(Attr.GetName(), Attr.GetRawValue());
		}
		Entity->EndNewAttrs();
	}

	Data::PDataArray Props;
	if (Desc.Get(Props, CStrID("Props")))
		for (UPTR i = 0; i < Props->GetCount(); ++i)
		{
			const Data::CData& PropID = Props->Get(i);
			if (PropID.IsA<int>()) EntityMgr->AttachProperty(*Entity, (Data::CFourCC)PropID.GetValue<int>());
			else if (PropID.IsA<CString>()) EntityMgr->AttachProperty(*Entity, PropID.GetValue<CString>());
			else Sys::Log("Failed to attach property #%d to entity %s at level %s\n", i, UID.CStr(), Level.GetID().CStr());
		}

	OK;
}
//---------------------------------------------------------------------

}