#include "EntityLoaderStatic.h"

//#include <Game/Mgr/StaticEnvManager.h>
#include <Game/EntityManager.h>

namespace Game
{
__ImplementClass(Game::CEntityLoaderStatic, 'ELST', Game::IEntityLoader);

bool CEntityLoaderStatic::Load(CStrID UID, CGameLevel& Level, Data::PParams Desc)
{
	//if (!StaticEnvMgr->AddEnvObject())
	//	EntityMgr->AttachEntity();
	OK;
}
//---------------------------------------------------------------------

} // namespace Loading
