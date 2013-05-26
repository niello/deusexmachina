#include "StaticMeshShape.h"

namespace Physics
{
__ImplementResourceClass(Physics::CStaticMeshShape, 'SMSH', Physics::CCollisionShape);

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
		pBtShape = NULL;
	}
	State = Resources::Rsrc_NotLoaded;
}
//---------------------------------------------------------------------
*/

}
