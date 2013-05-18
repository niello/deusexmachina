#include "EntityLoaderStatic.h"

#include <Game/StaticEnvManager.h>

namespace Game
{
__ImplementClass(Game::CEntityLoaderStatic, 'ELST', Game::IEntityLoader);

bool CEntityLoaderStatic::Load(CStrID UID, CGameLevel& Level, Data::PParams Desc)
{
	if (!Desc.IsValid() || !UID.IsValid()) FAIL;

	if (!StaticEnvMgr->CanEntityBeStatic(*Desc)) FAIL; //???or try to add as common?

	PStaticObject Obj = StaticEnvMgr->CreateStaticObject(UID, Level);
	if (!Obj.IsValid()) FAIL;

	Obj->Init(*Desc);

	OK;
}
//---------------------------------------------------------------------

}