#include "MeshData.h"

namespace Render
{
__ImplementClassNoFactory(Render::CMeshData, Resources::CResourceObject);

CMeshData::~CMeshData()
{
	if (pVBData) n_free_aligned(pVBData);
	if (pIBData) n_free_aligned(pIBData);
}
//---------------------------------------------------------------------

}