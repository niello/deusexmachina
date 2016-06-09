#include "TerrainRenderer.h"

#include <Render/RenderFwd.h>
#include <Render/RenderNode.h>
#include <Render/Terrain.h>
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
void CTerrainRenderer::PrepareNode(CRenderNode& Node)
{
	CTerrain* pTerrain = Node.pRenderable->As<CTerrain>();
	n_assert_dbg(pTerrain);

	//!!!can find once, outside the render loop! store somewhere in a persistent render node.
	//But this associates object and renderer, as other renderer may request other input set.
	//Node.pMaterial = pTerrain->Material.GetUnsafe();
	//Node.pTech = pTerrain->Material->GetEffect()->GetTechByInputSet(InputSet_CDLOD);
	Node.pMaterial = NULL;
	Node.pTech = NULL;
}
//---------------------------------------------------------------------

CArray<CRenderNode>::CIterator CTerrainRenderer::Render(CGPUDriver& GPU, CArray<CRenderNode>& RenderQueue, CArray<CRenderNode>::CIterator ItCurr)
{
	CArray<CRenderNode>::CIterator ItEnd = RenderQueue.End();
	while (ItCurr != ItEnd)
	{
		if (ItCurr->pRenderer != this) return ItCurr;

		Sys::DbgOut("CTerrain rendered\n");

		// Find tech
		// Render CTerrain

		++ItCurr;
	};

	return ItEnd;
}
//---------------------------------------------------------------------

}