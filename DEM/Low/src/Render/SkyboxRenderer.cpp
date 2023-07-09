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

bool CSkyboxRenderer::BeginRange(const CRenderContext& Context)
{
	_pCurrMaterial = nullptr;
	_pCurrTech = nullptr;
	_ConstWorldMatrix = {};
	OK;
}
//---------------------------------------------------------------------

void CSkyboxRenderer::Render(const CRenderContext& Context, IRenderable& Renderable/*, UPTR SortingKey*/)
{
	CSkybox& Skybox = static_cast<CSkybox&>(Renderable);

	const CTechnique* pTech = Context.pShaderTechCache[Skybox.ShaderTechIndex];
	n_assert_dbg(pTech);
	if (!pTech) return;

	if (pTech != _pCurrTech)
	{
		_pCurrTech = pTech;
		_pCurrMaterial = nullptr;
		_ConstWorldMatrix = pTech->GetParamTable().GetConstant(CStrID("WorldMatrix"));
	}

	auto pMaterial = Skybox.Material.Get();
	n_assert_dbg(pMaterial);
	if (!pMaterial) return;

	if (pMaterial != _pCurrMaterial)
	{
		_pCurrMaterial = pMaterial;
		n_verify_dbg(pMaterial->Apply());
	}

	UPTR LightCount = 0;
	const auto& Passes = pTech->GetPasses(LightCount);
	if (Passes.empty()) return;

	CGPUDriver& GPU = *Context.pGPU;

	if (_ConstWorldMatrix)
	{
		CShaderParamStorage PerInstance(pTech->GetParamTable(), GPU);
		matrix44 Tfm = Skybox.Transform;
		Tfm.set_translation(Context.CameraPosition);
		PerInstance.SetMatrix(_ConstWorldMatrix, Tfm);
		n_verify_dbg(PerInstance.Apply());
	}

	const CMesh* pMesh = Skybox.Mesh.Get();
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
}
//---------------------------------------------------------------------

void CSkyboxRenderer::EndRange(const CRenderContext& Context)
{
}
//---------------------------------------------------------------------

}
