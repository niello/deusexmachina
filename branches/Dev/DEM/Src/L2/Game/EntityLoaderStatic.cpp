#include "EntityLoaderStatic.h"

#include <Game/StaticEnvManager.h>
#include <Core/Factory.h>

namespace Game
{
__ImplementClass(Game::CEntityLoaderStatic, 'ELST', Game::IEntityLoader);

bool CEntityLoaderStatic::Load(CStrID UID, CGameLevel& Level, const Data::CParams& Desc)
{
	if (!UID.IsValid()) FAIL;

	if (!StaticEnvMgr->CanEntityBeStatic(Desc)) FAIL; //???or try to add as common?

	PStaticObject Obj = StaticEnvMgr->CreateStaticObject(UID, Level);
	if (Obj.IsNullPtr()) FAIL;

	Obj->Init(Desc);

	OK;
}
//---------------------------------------------------------------------

}