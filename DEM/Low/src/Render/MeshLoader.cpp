#include "MeshLoader.h"

#include <Render/Mesh.h>

namespace Resources
{
__ImplementClassNoFactory(Resources::CMeshLoader, Resources::CResourceLoader);

const Core::CRTTI& CMeshLoader::GetResultType() const
{
	return Render::CMesh::RTTI;
}
//---------------------------------------------------------------------

}