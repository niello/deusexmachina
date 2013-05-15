#include "EntityLoaderStatic.h"

#include <Game/Mgr/StaticEnvManager.h>
#include <Game/EntityManager.h>

namespace Game
{
__ImplementClassNoFactory(Game::CEntityLoaderStatic, Game::IEntityLoader);
__ImplementClass(Game::CEntityLoaderStatic);

bool CEntityLoaderStatic::Load(CStrID UID, CStrID LevelID, Data::PParams Desc)
{
	//if (!StaticEnvMgr->AddEnvObject(pTable, RowIdx))
	//	EntityMgr->AttachEntity(EntityFct->CreateEntityByCategory(Category, pTable, RowIdx));
	OK;
}
//---------------------------------------------------------------------

} // namespace Loading
