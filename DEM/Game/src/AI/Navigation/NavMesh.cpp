#include "NavMesh.h"

namespace DEM::AI
{
RTTI_CLASS_IMPL(DEM::AI::CNavMesh, Resources::CResourceObject);

CNavMesh::~CNavMesh()
{
	if (_pNavMesh) dtFreeNavMesh(_pNavMesh);
}
//---------------------------------------------------------------------

}
