#include "Terrain.h"
#include <Render/Material.h>
#include <Render/CDLODData.h>
#include <Render/Mesh.h>
#include <Render/Texture.h>
#include <Core/Factory.h>

namespace Render
{
__ImplementClass(Render::CTerrain, 'TERR', Render::IRenderable);

CTerrain::CTerrain() = default;
CTerrain::~CTerrain() = default;

PRenderable CTerrain::Clone()
{
	CTerrain* pCloned = n_new(CTerrain);
	pCloned->PatchMesh = PatchMesh;
	pCloned->QuarterPatchMesh = QuarterPatchMesh;
	pCloned->InvSplatSizeX = InvSplatSizeX;
	pCloned->InvSplatSizeZ = InvSplatSizeZ;
	return PRenderable(pCloned);
}
//---------------------------------------------------------------------

bool CTerrain::GetLocalAABB(CAABB& OutBox, UPTR LOD) const
{
	OutBox = CDLODData->GetAABB();
	OK;
}
//---------------------------------------------------------------------

}