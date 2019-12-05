#include "StaticMeshShape.h"

#include <Core/Factory.h>

namespace Physics
{
FACTORY_CLASS_IMPL(Physics::CStaticMeshShape, 'SMSH', Physics::CCollisionShape);

/*
bool CStaticMeshShape::Setup(btBvhTriangleMeshShape* pShape)
{
	if (!pShape)
	{
		State = Resources::Rsrc_Failed;
		FAIL;
	}

	pBtShape = pShape;
	State = Resources::Rsrc_Loaded;
	OK;
}
//---------------------------------------------------------------------

void CStaticMeshShape::Unload()
{
	n_assert(GetRefCount() <= 1); //!!!if unload when used, physics will crash!
	if (pBtShape)
	{
		delete pBtShape;
		pBtShape = nullptr;
	}
	State = Resources::Rsrc_NotLoaded;
}
//---------------------------------------------------------------------
*/

}
