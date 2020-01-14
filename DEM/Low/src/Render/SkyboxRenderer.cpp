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

bool CSkyboxRenderer::PrepareNode(CRenderNode& Node, const CRenderNodeContext& Context)
{
	CSkybox* pSkybox = Node.pRenderable->As<CSkybox>();
	n_assert_dbg(pSkybox);

	CMaterial* pMaterial = pSkybox->Material.Get(); //!!!Get by MaterialLOD / quality level!
	if (!pMaterial) FAIL;

	CEffect* pEffect = pMaterial->GetEffect();
	EEffectType EffType = pEffect->GetType();
	for (UPTR i = 0; i < Context.EffectOverrides.GetCount(); ++i)
	{
		if (Context.EffectOverrides.KeyAt(i) == EffType)
		{
			pEffect = Context.EffectOverrides.ValueAt(i).Get();
			break;
		}
	}

	if (!pEffect) FAIL;

	static const CStrID InputSet_Skybox("Skybox");

	Node.pMaterial = pMaterial;
	Node.pEffect = pEffect;
	Node.pTech = pEffect->GetTechByInputSet(InputSet_Skybox);
	if (!Node.pTech) FAIL;

	Node.pMesh = pSkybox->Mesh;
	Node.pGroup = pSkybox->Mesh->GetGroup(0, 0);

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
		CRenderNode* pRenderNode = *ItCurr;

		if (pRenderNode->pRenderer != this) return ItCurr;

		auto pMaterial = pRenderNode->pMaterial;
		if (pMaterial != pCurrMaterial)
		{
			n_assert_dbg(pMaterial);
			n_verify_dbg(pMaterial->Apply());
			pCurrMaterial = pMaterial;
		}

		const CTechnique* pTech = pRenderNode->pTech;
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

		const CMesh* pMesh = pRenderNode->pMesh;
		n_assert_dbg(pMesh);
		CVertexBuffer* pVB = pMesh->GetVertexBuffer().Get();
		n_assert_dbg(pVB);

		GPU.SetVertexLayout(pVB->GetVertexLayout());
		GPU.SetVertexBuffer(0, pVB);
		GPU.SetIndexBuffer(pMesh->GetIndexBuffer().Get());

		// Render sky at the far clipping plane
		// TODO: 0.f for reverse depth
		// TODO: control in a shader (VS output Z, W) instead of here? If so, don't forget to remove all SetViewport calls here.
		Render::CViewport VP;
		GPU.GetViewport(0, VP);
		const float OldMinDepth = VP.MinDepth;
		const float OldMaxDepth = VP.MaxDepth;
		VP.MinDepth = 1.0f;
		VP.MaxDepth = 1.0f;
		GPU.SetViewport(0, &VP);

		const CPrimitiveGroup* pGroup = pMesh->GetGroup(0);
		for (const auto& Pass : Passes)
		{
			GPU.SetRenderState(Pass);
			GPU.Draw(*pGroup);
		}

		VP.MinDepth = OldMinDepth;
		VP.MaxDepth = OldMaxDepth;
		GPU.SetViewport(0, &VP);

		++ItCurr;
	};

	return ItEnd;
}
//---------------------------------------------------------------------

}