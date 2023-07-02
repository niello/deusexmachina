#include "SkyboxRenderer.h"
#include <Render/GPUDriver.h>
#include <Render/RenderNode.h>
#include <Render/Skybox.h>
#include <Render/Material.h>
#include <Render/Effect.h>
#include <Core/Factory.h>

namespace Render
{
FACTORY_CLASS_IMPL(Render::CSkyboxRenderer, 'SBXR', Render::IRenderer);

CSkyboxRenderer::CSkyboxRenderer() = default;
//---------------------------------------------------------------------

bool CSkyboxRenderer::PrepareNode(IRenderable& Node, const CRenderNodeContext& Context)
{
	OK;
}
//---------------------------------------------------------------------

CRenderQueueIterator CSkyboxRenderer::Render(const CRenderContext& Context, CRenderQueue& RenderQueue, CRenderQueueIterator ItCurr)
{
	CGPUDriver& GPU = *Context.pGPU;

	CRenderQueueIterator ItEnd = RenderQueue.End();

	const CMaterial* pCurrMaterial = nullptr;
	const CTechnique* pCurrTech = nullptr;

	CShaderConstantParam ConstWorldMatrix;

	while (ItCurr != ItEnd)
	{
		IRenderable* pRenderNode = *ItCurr;

		if (pRenderNode->pRenderer != this) return ItCurr;

		CSkybox* pSkybox = pRenderNode->As<CSkybox>();
		n_assert_dbg(pSkybox);

		auto pMaterial = pSkybox->Material.Get();
		if (pMaterial != pCurrMaterial)
		{
			n_assert_dbg(pMaterial);
			n_verify_dbg(pMaterial->Apply());
			pCurrMaterial = pMaterial;
		}

		const CTechnique* pTech = Context.pShaderTechCache[pSkybox->ShaderTechIndex];
		if (pTech != pCurrTech)
		{
			pCurrTech = pTech;
			ConstWorldMatrix = pTech->GetParamTable().GetConstant(CStrID("WorldMatrix"));
		}

		UPTR LightCount = 0;
		const auto& Passes = pTech->GetPasses(LightCount);
		if (Passes.empty())
		{
			++ItCurr;
			continue;
		}

		CShaderParamStorage PerInstance(pTech->GetParamTable(), GPU);

		if (ConstWorldMatrix)
		{
			matrix44 Tfm = pRenderNode->Transform;
			Tfm.set_translation(Context.CameraPosition);
			PerInstance.SetMatrix(ConstWorldMatrix, Tfm);
		}
		n_verify_dbg(PerInstance.Apply());

		const CMesh* pMesh = pSkybox->Mesh.Get();
		n_assert_dbg(pMesh);
		CVertexBuffer* pVB = pMesh->GetVertexBuffer().Get();
		n_assert_dbg(pVB);

		GPU.SetVertexLayout(pVB->GetVertexLayout());
		GPU.SetVertexBuffer(0, pVB);
		GPU.SetIndexBuffer(pMesh->GetIndexBuffer().Get());

		// NB: rendered at the far clipping plane due to .xyww position swizzling in a shader
		const CPrimitiveGroup* pGroup = pMesh->GetGroup(0);
		for (const auto& Pass : Passes)
		{
			GPU.SetRenderState(Pass);
			GPU.Draw(*pGroup);
		}

		++ItCurr;
	};

	return ItEnd;
}
//---------------------------------------------------------------------

}
