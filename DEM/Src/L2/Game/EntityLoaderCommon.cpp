#include "EntityLoaderCommon.h"

#include <Game/EntityManager.h>
#include <Game/GameLevel.h>
#include <Data/DataArray.h>

namespace Game
{
__ImplementClass(Game::CEntityLoaderCommon, 'ELCM', Game::IEntityLoader);

bool CEntityLoaderCommon::Load(CStrID UID, CGameLevel& Level, Data::PParams Desc)
{
	n_assert(UID.IsValid());
	PEntity Entity = EntityMgr->CreateEntity(UID, Level);
	if (!Entity.IsValid()) FAIL;

	if (!Desc.IsValid()) OK;

	Data::PParams SubDesc;
	if (Desc->Get(SubDesc, CStrID("Attrs")))
		for (int i = 0; i < SubDesc->GetCount(); ++i)
		{
			const Data::CParam& Attr = SubDesc->Get(i);
			Entity->SetAttr(Attr.GetName(), Attr.GetRawValue());
		}

	Data::PDataArray Props;
	if (Desc->Get(Props, CStrID("Props")))
		for (int i = 0; i < Props->GetCount(); ++i)
		{
			const Data::CData& PropID = Props->Get(i);
			if (PropID.IsA<int>()) EntityMgr->AttachProperty(*Entity, (nFourCC)PropID.GetValue<int>());
			else if (PropID.IsA<nString>()) EntityMgr->AttachProperty(*Entity, PropID.GetValue<nString>());
			else n_printf("Failed to attach property #%d to entity %s at level %s\n", i, UID.CStr(), Level.GetID().CStr());
		}

	OK;
}
//---------------------------------------------------------------------

}