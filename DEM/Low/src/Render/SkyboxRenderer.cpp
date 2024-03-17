#include "SkyboxRenderer.h"
#include <Render/GPUDriver.h>
#include <Render/Skybox.h>
#include <Render/Material.h>
#include <Render/Effect.h>
#include <Core/Factory.h>

namespace Render
{
FACTORY_CLASS_IMPL(Render::CSkyboxRenderer, 'SBXR', Render::IRenderer);

CSkyboxRenderer::CSkyboxRenderer() = default;
//---------------------------------------------------------------------

CSkyboxRenderer::CSkyboxTechInterface* CSkyboxRenderer::GetTechInterface(const CTechnique* pTech)
{
	auto It = _TechInterfaces.find(pTech);
	if (It != _TechInterfaces.cend()) return &It->second;

	auto& TechInterface = _TechInterfaces[pTech];

	if (pTech->GetParamTable().HasParams())
	{
		static const CStrID sidWorldMatrix("WorldMatrix");
		TechInterface.PerInstanceParams = CShaderParamStorage(pTech->GetParamTable(), *_pGPU);
		TechInterface.ConstWorldMatrix = pTech->GetParamTable().GetConstant(sidWorldMatrix);
	}

	return &TechInterface;
}
//---------------------------------------------------------------------

bool CSkyboxRenderer::BeginRange(const CRenderContext& Context)
{
	_pCurrTech = nullptr;
	_pCurrTechInterface = nullptr;
	_pCurrMaterial = nullptr;
	_pGPU = Context.pGPU; // FIXME: could instead pass CRenderContext to accessing methods
	OK;
}
//---------------------------------------------------------------------

void CSkyboxRenderer::Render(const CRenderContext& Context, IRenderable& Renderable, IRenderModifier* pModifier)
{
	ZoneScoped;

	CSkybox& Skybox = static_cast<CSkybox&>(Renderable);

	const CTechnique* pTech = Context.pShaderTechCache[Skybox.ShaderTechIndex];
	if (!pTech) return;

	auto pMaterial = Skybox.Material.Get();
	n_assert_dbg(pMaterial);
	if (!pMaterial) return;

	const CMesh* pMesh = Skybox.Mesh.Get();
	n_assert_dbg(pMesh);
	if (!pMesh) return;

	if (pTech != _pCurrTech)
	{
		_pCurrTech = pTech;
		_pCurrTechInterface = GetTechInterface(pTech);
	}

	if (pMaterial != _pCurrMaterial)
	{
		_pCurrMaterial = pMaterial;
		n_verify_dbg(pMaterial->Apply());
	}

	if (_pCurrTechInterface->ConstWorldMatrix)
		_pCurrTechInterface->PerInstanceParams.SetMatrix(_pCurrTechInterface->ConstWorldMatrix, Skybox.Transform);

	if (pModifier) pModifier->ModifyPerInstanceShaderParams(_pCurrTechInterface->PerInstanceParams, 0);

	n_verify_dbg(_pCurrTechInterface->PerInstanceParams.Apply());

	CVertexBuffer* pVB = pMesh->GetVertexBuffer().Get();
	_pGPU->SetVertexLayout(pVB->GetVertexLayout());
	_pGPU->SetVertexBuffer(0, pVB);
	_pGPU->SetIndexBuffer(pMesh->GetIndexBuffer().Get());

	// NB: rendered at the far clipping plane due to .xyww position swizzling in a shader
	const CPrimitiveGroup* pGroup = pMesh->GetGroup(0);
	for (const auto& Pass : pTech->GetPasses())
	{
		_pGPU->SetRenderState(Pass);
		_pGPU->Draw(*pGroup);
	}
}
//---------------------------------------------------------------------

void CSkyboxRenderer::EndRange(const CRenderContext& Context)
{
}
//---------------------------------------------------------------------

}
