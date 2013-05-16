#include "EntityLoaderStatic.h"

//#include <Game/Mgr/StaticEnvManager.h>
#include <Game/EntityManager.h>

namespace Game
{
__ImplementClass(Game::CEntityLoaderStatic, 'ELST', Game::IEntityLoader);

bool CEntityLoaderStatic::Load(CStrID UID, CGameLevel& Level, Data::PParams Desc)
{
	//if (!StaticEnvMgr->AddEnvObject(pTable, RowIdx))
	//	EntityMgr->AttachEntity(EntityFct->CreateEntityByCategory(Category, pTable, RowIdx));
	OK;
}
//---------------------------------------------------------------------

} // namespace Loading
