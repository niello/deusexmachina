#include "TerrainRenderer.h"

#include <Render/RenderNode.h>
#include <Render/GPUDriver.h>
#include <Render/Terrain.h>
#include <Render/Material.h>
#include <Render/Effect.h>
#include <Core/Factory.h>

namespace Render
{
__ImplementClass(Render::CTerrainRenderer, 'TRNR', Render::IRenderer);

CTerrainRenderer::CTerrainRenderer()
{
	// Setup dynamic enumeration
	InputSet_CDLOD = RegisterShaderInputSetID(CStrID("CDLOD"));
}
//---------------------------------------------------------------------

//???return bool, if false, remove node from queue
//(array tail removal is very fast in CArray, can even delay removal in a case next RQ node will be added inplace)?
bool CTerrainRenderer::PrepareNode(CRenderNode& Node, const CRenderNodeContext& Context)
{
	CTerrain* pTerrain = Node.pRenderable->As<CTerrain>();
	n_assert_dbg(pTerrain);

	CMaterial* pMaterial = pTerrain->GetMaterial(); //!!!Get by MaterialLOD!
	if (!pMaterial) FAIL;

	CEffect* pEffect = pMaterial->GetEffect();
	EEffectType EffType = pEffect->GetType();
	if (Context.pEffectOverrides)
		for (UPTR i = 0; i < Context.pEffectOverrides->GetCount(); ++i)
			if (Context.pEffectOverrides->KeyAt(i) == EffType)
			{
				pEffect = Context.pEffectOverrides->ValueAt(i).GetUnsafe();
				break;
			}

	if (!pEffect) FAIL;

	Node.pMaterial = pMaterial;
	Node.pTech = pEffect->GetTechByInputSet(InputSet_CDLOD);
	if (!Node.pTech) FAIL;

	Node.pMesh = pTerrain->GetPatchMesh();
	Node.pGroup = pTerrain->GetPatchMesh()->GetGroup(0, 0); // For sorting, different terrain objects with the same mesh will be rendered sequentially

	OK;
}
//---------------------------------------------------------------------

CArray<CRenderNode>::CIterator CTerrainRenderer::Render(CGPUDriver& GPU, CArray<CRenderNode>& RenderQueue, CArray<CRenderNode>::CIterator ItCurr)
{
	const bool GPUSupportsVSTextureLinearFiltering = GPU.CheckCaps(Caps_VSTexFiltering_Linear);

	CArray<CRenderNode>::CIterator ItEnd = RenderQueue.End();
	while (ItCurr != ItEnd)
	{
		if (ItCurr->pRenderer != this) return ItCurr;

		if (!GPUSupportsVSTextureLinearFiltering) continue;

		Sys::DbgOut("CTerrain rendered\n");

		// Find tech
		// Render CTerrain

		++ItCurr;
	};

	return ItEnd;
}
//---------------------------------------------------------------------

}