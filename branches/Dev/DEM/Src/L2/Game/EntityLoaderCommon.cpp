#include "EntityLoaderCommon.h"

#include <Game/EntityManager.h>
#include <Data/DataArray.h>

namespace Game
{
__ImplementClassNoFactory(Game::CEntityLoaderCommon, Game::IEntityLoader);
__ImplementClass(Game::CEntityLoaderCommon);

bool CEntityLoaderCommon::Load(CStrID UID, CStrID LevelID, Data::PParams Desc)
{
	n_assert(UID.IsValid());
	PEntity Entity = EntityMgr->CreateEntity(UID, LevelID);
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
			const nString& ClassName = Props->Get
			EntityMgr->AttachProperty(*Entity, RTTI);
		}

	OK;
}
//---------------------------------------------------------------------

} // namespace Loading
