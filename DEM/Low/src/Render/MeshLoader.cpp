#include "MeshLoader.h"

#include <Render/Mesh.h>
#include <Render/GPUDriver.h>

namespace Resources
{
CMeshLoader::~CMeshLoader() {}

const Core::CRTTI& CMeshLoader::GetResultType() const
{
	return Render::CMesh::RTTI;
}
//---------------------------------------------------------------------

}