#include "MeshData.h"

namespace Render
{
__ImplementClassNoFactory(Render::CMeshData, Resources::CResourceObject);

CMeshData::~CMeshData()
{
	n_free_aligned(pVBData);
	n_free_aligned(pIBData);
}
//---------------------------------------------------------------------

}