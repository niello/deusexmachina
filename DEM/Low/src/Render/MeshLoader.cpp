#include "MeshLoader.h"

#include <Render/Mesh.h>
#include <Render/GPUDriver.h>

namespace Resources
{
__ImplementClassNoFactory(Resources::CMeshLoader, Resources::CResourceLoader);

CMeshLoader::~CMeshLoader() {}

const Core::CRTTI& CMeshLoader::GetResultType() const
{
	return Render::CMesh::RTTI;
}
//---------------------------------------------------------------------

}