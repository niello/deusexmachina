#include "EntityLoader.h"

#include <Loading/EntityFactory.h>
#include <Game/Mgr/EntityManager.h>

namespace Loading
{
ImplementRTTI(Loading::CEntityLoader, Loading::CEntityLoaderBase);
ImplementFactory(Loading::CEntityLoader);

using namespace Game;

bool CEntityLoader::Load(CStrID Category, DB::CValueTable* pTable, int RowIdx)
{
	n_assert(pTable);
	PEntity GameEntity = EntityFct->CreateEntityByCategory(Category, pTable, RowIdx);
	EntityMgr->AttachEntity(GameEntity);
	OK;
}
//---------------------------------------------------------------------

} // namespace Loading
