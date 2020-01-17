#include "Model.h"
#include <Render/Mesh.h>
#include <Render/Material.h>
#include <Core/Factory.h>

namespace Render
{
FACTORY_CLASS_IMPL(Render::CModel, 'MODL', Render::IRenderable);

PRenderable CModel::Clone()
{
	CModel* pCloned = n_new(CModel);
	pCloned->Mesh = Mesh;
	pCloned->Material = Material;
	pCloned->MeshGroupIndex = MeshGroupIndex;
	pCloned->BoneIndices.RawCopyFrom(BoneIndices.GetPtr(), BoneIndices.GetCount());
	return PRenderable(pCloned);
}
//---------------------------------------------------------------------

bool CModel::GetLocalAABB(CAABB& OutBox, UPTR LOD) const
{
	if (!Mesh) FAIL;

	const CPrimitiveGroup* pGroup =  Mesh->GetGroup(MeshGroupIndex, LOD);
	if (!pGroup) FAIL;

	OutBox = pGroup->AABB;
	OK;
}
//---------------------------------------------------------------------

}