#include "SkyboxRenderer.h"

#include <Render/RenderNode.h>
#include <Render/Skybox.h>
#include <Render/Material.h>
#include <Render/Effect.h>
/*
#include <Render/GPUDriver.h>
#include <Render/Terrain.h>
#include <Render/EffectConstSetValues.h>
#include <Math/Sphere.h>
*/
#include <Core/Factory.h>

namespace Render
{
__ImplementClass(Render::CSkyboxRenderer, 'SBXR', Render::IRenderer);

CSkyboxRenderer::CSkyboxRenderer()
{
	// Setup dynamic enumeration
	InputSet_Skybox = RegisterShaderInputSetID(CStrID("Skybox"));
}
//---------------------------------------------------------------------

bool CSkyboxRenderer::PrepareNode(CRenderNode& Node, const CRenderNodeContext& Context)
{
	CSkybox* pSkybox = Node.pRenderable->As<CSkybox>();
	n_assert_dbg(pSkybox);

	CMaterial* pMaterial = pSkybox->GetMaterial(); //!!!Get by MaterialLOD!
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
	Node.pEffect = pEffect;
	Node.pTech = pEffect->GetTechByInputSet(InputSet_Skybox);
	if (!Node.pTech) FAIL;

	Node.pMesh = pSkybox->GetMesh();
	Node.pGroup = pSkybox->GetMesh()->GetGroup(0, 0);

	OK;
}
//---------------------------------------------------------------------

CArray<CRenderNode>::CIterator CSkyboxRenderer::Render(const CRenderContext& Context, CArray<CRenderNode>& RenderQueue, CArray<CRenderNode>::CIterator ItCurr)
{
	CGPUDriver& GPU = *Context.pGPU;

	CArray<CRenderNode>::CIterator ItEnd = RenderQueue.End();

	while (ItCurr != ItEnd)
	{
		if (ItCurr->pRenderer != this) return ItCurr;

		CSkybox* pSkybox = ItCurr->pRenderable->As<CSkybox>();

		//!!!DBG TMP!
		Sys::DbgOut("CSkybox rendered\n");

		++ItCurr;
	};

	return ItEnd;
}
//---------------------------------------------------------------------

}