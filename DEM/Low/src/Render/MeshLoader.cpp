#include "MeshLoader.h"

#include <Render/Mesh.h>
#include <Render/GPUDriver.h>

namespace Resources
{
__ImplementClassNoFactory(Resources::CMeshLoader, Resources::IResourceCreator);

CMeshLoader::~CMeshLoader() {}

const Core::CRTTI& CMeshLoader::GetResultType() const
{
	return Render::CMesh::RTTI;
}
//---------------------------------------------------------------------

}