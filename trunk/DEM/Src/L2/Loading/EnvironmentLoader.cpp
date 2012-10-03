#include "EnvironmentLoader.h"

#include <Loading/EntityFactory.h>
#include <Game/Mgr/StaticEnvManager.h>
#include <Game/Mgr/EntityManager.h>

namespace Loading
{
ImplementRTTI(Loading::CEnvironmentLoader, Loading::CEntityLoaderBase);
ImplementFactory(Loading::CEnvironmentLoader);

bool CEnvironmentLoader::Load(CStrID Category, DB::CValueTable* pTable, int RowIdx)
{
	n_assert(pTable);
	if (!StaticEnvMgr->AddEnvObject(pTable, RowIdx))
		EntityMgr->AttachEntity(EntityFct->CreateEntityByCategory(Category, pTable, RowIdx));
	OK;
}
//---------------------------------------------------------------------

} // namespace Loading
