#include "ModelRenderer.h"

#include <Render/RenderFwd.h>
#include <Render/RenderNode.h>
#include <Render/Model.h>
#include <Render/Material.h>
#include <Render/Effect.h>
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
void CModelRenderer::PrepareNode(CRenderNode& Node, UPTR MeshLOD, UPTR MaterialLOD)
{
	CModel* pModel = Node.pRenderable->As<CModel>();
	n_assert_dbg(pModel);

	//!!!can find once, outside the render loop! store somewhere in a persistent render node.
	//But this associates object and renderer, as other renderer may request other input set.
	Node.pMaterial = pModel->Material.GetUnsafe(); //!!!Get by MaterialLOD!
	Node.pTech = pModel->Material->GetEffect()->GetTechByInputSet(Node.pSkinPalette ? InputSet_ModelSkinned : InputSet_Model);
	Node.pGroup = pModel->Mesh->GetGroup(pModel->MeshGroupIndex, MeshLOD);
}
//---------------------------------------------------------------------

// Optimal sorting for the color phase is Material-Tech-Mesh-Group for opaque and then BtF for transparent
CArray<CRenderNode>::CIterator CModelRenderer::Render(CGPUDriver& GPU, CArray<CRenderNode>& RenderQueue, CArray<CRenderNode>::CIterator ItCurr)
{
	const CMaterial* pCurrMaterial = NULL;
	const CMesh* pCurrMesh = NULL;

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
			pMaterial->Apply(GPU);
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
			for (; ItCurr != ItInstEnd; ++ItCurr)
			{
				// Per-instance params
				// Write per-instance params into tech params
				//HConst hWorld = pTech->GetParam(CStrID("WorldMatrix")); //???or find index and then reference by index?
				//!!!search in fallback materials if not found!
				//set tech params (feed shader according to an input set)
				//GPU.SetShaderConstant(TmpCB, hWorld, 0, ItCurr->Transform.m, sizeof(matrix44));
				//if (ItCurr->pSkinPalette) GPU.SetShaderConstant(TmpCB, hSkinPalette, 0, ItCurr->pSkinPalette, sizeof(matrix44) * ItCurr->BoneCount);

				//???loop by pass, then by instance? possibly less render state switches, but possibly more data binding. Does order matter?
				for (UPTR i = 0; i < pPasses->GetCount(); ++i)
				{
					GPU.SetRenderState((*pPasses)[i]);
					GPU.Draw(*pGroup);
				}

				Sys::DbgOut("CModel rendered non-instanced, tech '%s', group 0x%X\n", ItCurr->pTech->GetName().CStr(), pGroup);
			}
		}

	};

	return ItEnd;
}
//---------------------------------------------------------------------

}