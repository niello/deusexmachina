#include "ModelRenderer.h"

#include <Render/RenderFwd.h>
#include <Render/RenderNode.h>
#include <Render/Model.h>
#include <Render/Material.h>
#include <Render/Effect.h>
#include <Render/EffectConstSetValues.h>
#include <Render/Mesh.h>
#include <Render/GPUDriver.h>
#include <Core/Factory.h>

namespace Render
{
__ImplementClass(Render::CModelRenderer, 'MDLR', Render::IRenderer);

CModelRenderer::CModelRenderer()
{
	// Setup dynamic enumeration
	InputSet_Model = RegisterShaderInputSetID(CStrID("Model"));
	InputSet_ModelSkinned = RegisterShaderInputSetID(CStrID("ModelSkinned"));
	InputSet_ModelInstanced = RegisterShaderInputSetID(CStrID("ModelInstanced"));
}
//---------------------------------------------------------------------

//???return bool, if false, remove node from queue
//(array tail removal is very fast in CArray, can even delay removal in a case next RQ node will be added inplace)?
bool CModelRenderer::PrepareNode(CRenderNode& Node, const CRenderNodeContext& Context)
{
	CModel* pModel = Node.pRenderable->As<CModel>();
	n_assert_dbg(pModel);

	//!!!can find once, outside the render loop! store somewhere in a persistent render node.
	//But this associates object and renderer, as other renderer may request other input set.
	CMaterial* pMaterial = pModel->Material.GetUnsafe(); //!!!Get by MaterialLOD!
	if (!pMaterial) FAIL;

	EEffectType EffType = pMaterial->GetEffect()->GetType();
	if (Context.pMaterialOverrides)
		for (UPTR i = 0; i < Context.pMaterialOverrides->GetCount(); ++i)
			if (Context.pMaterialOverrides->KeyAt(i) == EffType)
			{
				pMaterial = Context.pMaterialOverrides->ValueAt(i).GetUnsafe();
				break;
			}

	Node.pMaterial = pMaterial;
	Node.pTech = pModel->Material->GetEffect()->GetTechByInputSet(Node.pSkinPalette ? InputSet_ModelSkinned : InputSet_Model);
	Node.pMesh = pModel->Mesh.GetUnsafe();
	Node.pGroup = pModel->Mesh->GetGroup(pModel->MeshGroupIndex, Context.MeshLOD);

	OK;
}
//---------------------------------------------------------------------

// Optimal sorting for the color phase is Tech-Material-Mesh-Group for opaque and then BtF for transparent.
// Tech is sorted before Material because it is more likely that many materials will be rendered with the same
// single-pass tech, than that the same material will be used with many different techs. We have great chances
// to set render state only once as our tech is single-pass, and to render many materials without switching it,
// just rebinding constants, resources and samplers.
CArray<CRenderNode>::CIterator CModelRenderer::Render(CGPUDriver& GPU, CArray<CRenderNode>& RenderQueue, CArray<CRenderNode>::CIterator ItCurr)
{
	const CMaterial* pCurrMaterial = NULL;
	const CMesh* pCurrMesh = NULL;
	const CTechnique* pCurrTech = NULL;

	const CEffectConstant* pConstWorldMatrix = NULL; // Model, ModelSkinned
	const CEffectConstant* pConstSkinPalette = NULL; // ModelSkinned

	CArray<CRenderNode>::CIterator ItEnd = RenderQueue.End();
	while (ItCurr != ItEnd)
	{
		if (ItCurr->pRenderer != this) return ItCurr;

		const CTechnique* pTech = ItCurr->pTech;
		const CPrimitiveGroup* pGroup = ItCurr->pGroup;
		n_assert_dbg(pGroup && pTech);

		// Apply material, if changed

		const CMaterial* pMaterial = ItCurr->pMaterial;
		if (pMaterial != pCurrMaterial)
		{
			n_assert_dbg(pMaterial);
			n_verify_dbg(pMaterial->Apply(GPU));
			pCurrMaterial = pMaterial;

			//!!!DBG TMP!
			Sys::DbgOut("Material changed: 0x%X\n", pMaterial);
		}

		// Apply geometry, if changed

		CModel* pModel = ItCurr->pRenderable->As<CModel>();
		const CMesh* pMesh = pModel->Mesh.GetUnsafe();
		if (pMesh != pCurrMesh)
		{
			n_assert_dbg(pMesh);
			CVertexBuffer* pVB = pMesh->GetVertexBuffer().GetUnsafe();
			n_assert_dbg(pVB);
			GPU.SetVertexLayout(pVB->GetVertexLayout());
			GPU.SetVertexBuffer(0, pVB);
			GPU.SetIndexBuffer(pModel->Mesh->GetIndexBuffer().GetUnsafe());
			pCurrMesh = pMesh;

			//!!!DBG TMP!
			Sys::DbgOut("Mesh changed: 0x%X\n", pMesh);
		}

		// Gather instances (no skinned instancing supported)

		bool HardwareInstancing = false;
		CArray<CRenderNode>::CIterator ItInstEnd = ItCurr + 1;
		if (!ItCurr->pSkinPalette)
		{
			while (ItInstEnd != ItEnd &&
				   ItInstEnd->pRenderer == this &&
				   ItInstEnd->pMaterial == pMaterial &&
				   ItInstEnd->pTech == pTech &&
				   ItInstEnd->pGroup == pGroup &&
				   !ItInstEnd->pSkinPalette)
			{
				// We don't try to find an instanced tech version here, and don't break if
				// it is not found, because if we did, the next object will try to do all
				// this again, not knowing that there is no chance to success. If there is
				// no instanced tech version, we render instances in a loop manually instead.
				++ItInstEnd;
			}

			if (ItInstEnd - ItCurr > 1)
			{
				const CTechnique* pInstancedTech = pMaterial->GetEffect()->GetTechByInputSet(InputSet_ModelInstanced);
				if (pInstancedTech)
				{
					pTech = pInstancedTech;
					HardwareInstancing = true;
				}
			}
		}

		UPTR LightCount = 0;
		//!!!calc lights!
		//for instances may select maximum of light counts and use black lights for ones not used, or use per-instance count and dynamic loop
		//tech with a dynamic light count will be found at LightCount = 0

		const CPassList* pPasses = pTech->GetPasses(LightCount);
		n_assert_dbg(pPasses); // To test if it could happen at all
		if (!pPasses)
		{
			ItCurr = ItInstEnd;
			continue;
		}

		if (HardwareInstancing)
		{
			// Per-instance params
			// Write per-instance params into an instance buffer whatever form it takes

			for (UPTR i = 0; i < pPasses->GetCount(); ++i)
			{
				GPU.SetRenderState((*pPasses)[i]);
				GPU.Draw(*pGroup/*, ItInstEnd - ItCurr*/);
			}

			Sys::DbgOut("CModel rendered instanced, tech '%s', group 0x%X, instances: %d\n", ItCurr->pTech->GetName().CStr(), pGroup, (ItInstEnd - ItCurr));

			ItCurr = ItInstEnd;
		}
		else
		{
			//!!!DBG TMP!
			if (ItInstEnd - ItCurr > 1) Sys::DbgOut("Instancing might be possible, instances: %d\n", (ItInstEnd - ItCurr));

			if (pTech != pCurrTech)
			{
				pConstWorldMatrix = pTech->GetConstant(CStrID("WorldMatrix"));
				pConstSkinPalette = pTech->GetConstant(CStrID("SkinPalette"));
				pCurrTech = pTech;

				//!!!DBG TMP!
				Sys::DbgOut("Tech params requested by ID\n");
			}

			for (; ItCurr != ItInstEnd; ++ItCurr)
			{
				// Per-instance params

				//???use persistent, create once and store associatively Tech->Values?
				CEffectConstSetValues PerInstanceConstValues;
				PerInstanceConstValues.SetGPU(&GPU);

				if (pConstWorldMatrix)
				{
					PerInstanceConstValues.RegisterConstantBuffer(pConstWorldMatrix->BufferHandle, NULL);
					PerInstanceConstValues.SetConstantValue(pConstWorldMatrix, 0, ItCurr->Transform.m, sizeof(matrix44));
				}

				if (pConstSkinPalette && ItCurr->pSkinPalette)
				{
					PerInstanceConstValues.RegisterConstantBuffer(pConstSkinPalette->BufferHandle, NULL);
					if (pModel->BoneIndices.GetCount())
					{
						for (UPTR BoneIdxIdx = 0; BoneIdxIdx < pModel->BoneIndices.GetCount(); ++BoneIdxIdx)
						{
							int BoneIdx = pModel->BoneIndices[BoneIdxIdx];
							PerInstanceConstValues.SetConstantValue(pConstSkinPalette, BoneIdx, ItCurr->pSkinPalette + sizeof(matrix44) * BoneIdx, sizeof(matrix44));
						}
					}
					else
					{
						PerInstanceConstValues.SetConstantValue(pConstSkinPalette, 0, ItCurr->pSkinPalette, sizeof(matrix44) * ItCurr->BoneCount);
					}
				}

				PerInstanceConstValues.ApplyConstantBuffers();

				// Rendering

				//???loop by pass, then by instance? possibly less render state switches, but possibly more data binding. Does order matter?
				for (UPTR i = 0; i < pPasses->GetCount(); ++i)
				{
					GPU.SetRenderState((*pPasses)[i]);
					GPU.Draw(*pGroup);
				}

				Sys::DbgOut("CModel rendered non-instanced, tech '%s', group 0x%X, primitives: %d\n", ItCurr->pTech->GetName().CStr(), pGroup, pGroup->IndexCount);
			}
		}

	};

	return ItEnd;
}
//---------------------------------------------------------------------

}