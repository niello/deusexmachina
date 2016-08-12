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

	InstanceDataDecl.SetSize(4);

	// World matrix
	for (U32 i = 0; i < 4; ++i)
	{
		CVertexComponent& Cmp = InstanceDataDecl[i];
		Cmp.Semantic = VCSem_TexCoord;
		Cmp.UserDefinedName = NULL;
		Cmp.Index = i + 4;
		Cmp.Format = VCFmt_Float32_4;
		Cmp.Stream = INSTANCE_BUFFER_STREAM_INDEX;
		Cmp.OffsetInVertex = DEM_VERTEX_COMPONENT_OFFSET_DEFAULT;
		Cmp.PerInstanceData = true;
	}

	//!!!DBG TMP! //???where to define?
	MaxInstanceCount = 30;
}
//---------------------------------------------------------------------

bool CModelRenderer::PrepareNode(CRenderNode& Node, const CRenderNodeContext& Context)
{
	CModel* pModel = Node.pRenderable->As<CModel>();
	n_assert_dbg(pModel);

	CMaterial* pMaterial = pModel->Material.GetUnsafe(); //!!!Get by MaterialLOD!
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
	Node.pTech = pEffect->GetTechByInputSet(Node.pSkinPalette ? InputSet_ModelSkinned : InputSet_Model);
	if (!Node.pTech) FAIL;

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
	CVertexLayout* pVL = NULL;
	CVertexLayout* pVLInstanced = NULL;

	const CEffectConstant* pConstWorldMatrix = NULL;	// Model, ModelSkinned
	const CEffectConstant* pConstSkinPalette = NULL;	// ModelSkinned
	const CEffectConstant* pConstInstanceData = NULL;	// ModelInstanced

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
			GPU.SetVertexBuffer(0, pVB);
			GPU.SetIndexBuffer(pModel->Mesh->GetIndexBuffer().GetUnsafe());
			pCurrMesh = pMesh;

			pVL = pVB->GetVertexLayout();
			pVLInstanced = NULL;

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

		// Gather lights and select corresponding tech variation

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
			if (pTech != pCurrTech)
			{
				pConstInstanceData = pTech->GetConstant(CStrID("InstanceData"));
				pCurrTech = pTech;

				//!!!DBG TMP!
				Sys::DbgOut("Tech params requested by ID\n");
			}

			if (pConstInstanceData)
			{
				UPTR MaxInstanceCountConst = pConstInstanceData->Desc.ElementCount;
				n_assert_dbg(MaxInstanceCountConst > 1);

				//!!!DBG TMP!
				if ((ItInstEnd - ItCurr) > (IPTR)MaxInstanceCountConst)
					Sys::DbgOut("Instance buffer overflow (%d of %d), data will be split\n", (ItInstEnd - ItCurr), MaxInstanceCountConst);

				CEffectConstSetValues PerInstanceConstValues;
				PerInstanceConstValues.SetGPU(&GPU);
				PerInstanceConstValues.RegisterConstantBuffer(pConstInstanceData->Desc.BufferHandle, NULL);

				UPTR InstanceCount = 0;
				while (ItCurr != ItInstEnd)
				{
					PerInstanceConstValues.SetConstantValue(pConstInstanceData, InstanceCount, ItCurr->Transform.m, sizeof(matrix44));
					++InstanceCount;
					++ItCurr;

					if (InstanceCount == MaxInstanceCountConst)
					{
						PerInstanceConstValues.ApplyConstantBuffers();
						for (UPTR i = 0; i < pPasses->GetCount(); ++i)
						{
							GPU.SetRenderState((*pPasses)[i]);
							GPU.DrawInstanced(*pGroup, InstanceCount);
						}
						InstanceCount = 0;
						if (ItCurr == ItInstEnd) break;
					}
				}

				if (InstanceCount)
				{
					PerInstanceConstValues.ApplyConstantBuffers();
					for (UPTR i = 0; i < pPasses->GetCount(); ++i)
					{
						GPU.SetRenderState((*pPasses)[i]);
						GPU.DrawInstanced(*pGroup, InstanceCount);
					}
				}
			}
			else
			{
				n_assert_dbg(MaxInstanceCount > 1);

				//!!!DBG TMP!
				if ((ItInstEnd - ItCurr) > (IPTR)MaxInstanceCount)
					Sys::DbgOut("Instance buffer overflow (%d of %d), data will be split\n", (ItInstEnd - ItCurr), MaxInstanceCount);

				// We create this buffer lazy because for D3D11 possibility is high to use only constant-based instancing
				if (InstanceVB.IsNullPtr())
				{
					PVertexLayout VLInstanceData = GPU.CreateVertexLayout(InstanceDataDecl.GetPtr(), InstanceDataDecl.GetCount());
					InstanceVB = GPU.CreateVertexBuffer(*VLInstanceData, MaxInstanceCount, Access_CPU_Write | Access_GPU_Read);
				}

				if (!pVLInstanced)
				{
					IPTR VLIdx = InstancedLayouts.FindIndex(pVL);
					if (VLIdx == INVALID_INDEX)
					{
						UPTR BaseComponentCount = pVL->GetComponentCount();
						UPTR DescComponentCount = BaseComponentCount + InstanceDataDecl.GetCount();
						CVertexComponent* pInstancedDecl = (CVertexComponent*)_malloca(DescComponentCount * sizeof(CVertexComponent));
						memcpy(pInstancedDecl, pVL->GetComponent(0), BaseComponentCount * sizeof(CVertexComponent));
						memcpy(pInstancedDecl + BaseComponentCount, InstanceDataDecl.GetPtr(), InstanceDataDecl.GetCount() * sizeof(CVertexComponent));

						PVertexLayout VLInstanced = GPU.CreateVertexLayout(pInstancedDecl, DescComponentCount);

						_freea(pInstancedDecl);

						pVLInstanced = VLInstanced.GetUnsafe();
						n_assert_dbg(pVLInstanced);
						InstancedLayouts.Add(pVL, VLInstanced);
					}
					else pVLInstanced = InstancedLayouts.ValueAt(VLIdx).GetUnsafe();
				}

				GPU.SetVertexLayout(pVLInstanced);
				GPU.SetVertexBuffer(INSTANCE_BUFFER_STREAM_INDEX, InstanceVB.GetUnsafe());

				void* pInstData;
				n_verify(GPU.MapResource(&pInstData, *InstanceVB, Map_WriteDiscard)); //???use big buffer + no overwrite?
				UPTR InstanceCount = 0;
				while (ItCurr != ItInstEnd)
				{
					memcpy(pInstData, ItCurr->Transform.m, sizeof(matrix44));
					pInstData = (char*)pInstData + sizeof(matrix44);
					++InstanceCount;
					++ItCurr;

					if (InstanceCount == MaxInstanceCount)
					{
						GPU.UnmapResource(*InstanceVB);
						for (UPTR i = 0; i < pPasses->GetCount(); ++i)
						{
							GPU.SetRenderState((*pPasses)[i]);
							GPU.DrawInstanced(*pGroup, InstanceCount);
						}
						InstanceCount = 0;
						if (ItCurr == ItInstEnd) break;
						n_verify(GPU.MapResource(&pInstData, *InstanceVB, Map_WriteDiscard)); //???use big buffer + no overwrite?
					}
				}

				if (InstanceCount)
				{
					GPU.UnmapResource(*InstanceVB);
					for (UPTR i = 0; i < pPasses->GetCount(); ++i)
					{
						GPU.SetRenderState((*pPasses)[i]);
						GPU.DrawInstanced(*pGroup, InstanceCount);
					}
				}
			}

			Sys::DbgOut("CModel rendered instanced, tech '%s', group 0x%X, instances: %d\n", pTech->GetName().CStr(), pGroup, (ItInstEnd - ItCurr));
		}
		else
		{
			if (pTech != pCurrTech)
			{
				pConstWorldMatrix = pTech->GetConstant(CStrID("WorldMatrix"));
				pConstSkinPalette = pTech->GetConstant(CStrID("SkinPalette"));
				pCurrTech = pTech;

				//!!!DBG TMP!
				Sys::DbgOut("Tech params requested by ID\n");
			}

			GPU.SetVertexLayout(pVL);

			for (; ItCurr != ItInstEnd; ++ItCurr)
			{
				// Per-instance params

				//???use persistent, create once and store associatively Tech->Values?
				CEffectConstSetValues PerInstanceConstValues;
				PerInstanceConstValues.SetGPU(&GPU);

				if (pConstWorldMatrix)
				{
					PerInstanceConstValues.RegisterConstantBuffer(pConstWorldMatrix->Desc.BufferHandle, NULL);
					PerInstanceConstValues.SetConstantValue(pConstWorldMatrix, 0, ItCurr->Transform.m, sizeof(matrix44));
				}

				if (pConstSkinPalette && ItCurr->pSkinPalette)
				{
					PerInstanceConstValues.RegisterConstantBuffer(pConstSkinPalette->Desc.BufferHandle, NULL);
					if (pConstSkinPalette->Desc.Flags & Const_ColumnMajor) //???hide in a class/function?
					{
						// Transpose and truncate if necessary
						U32 Columns = pConstSkinPalette->Desc.Columns;
						U32 Rows = pConstSkinPalette->Desc.Rows;
						UPTR MatrixSize = Columns * Rows * sizeof(float);
						float* pTransposedData = (float*)_malloca(MatrixSize);

						if (pModel->BoneIndices.GetCount())
						{
							UPTR BoneCount = n_min(pModel->BoneIndices.GetCount(), pConstSkinPalette->Desc.ElementCount);
							for (UPTR BoneIdxIdx = 0; BoneIdxIdx < BoneCount; ++BoneIdxIdx)
							{
								float* pCurrData = pTransposedData;
								const matrix44* pBoneMatrix = ItCurr->pSkinPalette + pModel->BoneIndices[BoneIdxIdx];
								for (U32 Col = 0; Col < Columns; ++Col)
									for (U32 Row = 0; Row < Rows; ++Row)
									{
										*pCurrData = pBoneMatrix->m[Row][Col];
										++pCurrData;
									}
								PerInstanceConstValues.SetConstantValue(pConstSkinPalette, BoneIdxIdx, pTransposedData, MatrixSize);
							}
						}
						else
						{
							UPTR BoneCount = n_min(ItCurr->BoneCount, pConstSkinPalette->Desc.ElementCount);
							for (UPTR BoneIdx = 0; BoneIdx < BoneCount; ++BoneIdx)
							{
								float* pCurrData = pTransposedData;
								const matrix44* pBoneMatrix = ItCurr->pSkinPalette + BoneIdx;
								for (U32 Col = 0; Col < Columns; ++Col)
									for (U32 Row = 0; Row < Rows; ++Row)
									{
										*pCurrData = pBoneMatrix->m[Row][Col];
										++pCurrData;
									}
								PerInstanceConstValues.SetConstantValue(pConstSkinPalette, BoneIdx, pTransposedData, MatrixSize);
							}
						}

						_freea(pTransposedData);
					}
					else
					{
						if (pModel->BoneIndices.GetCount())
						{
							UPTR BoneCount = n_min(pModel->BoneIndices.GetCount(), pConstSkinPalette->Desc.ElementCount);
							for (UPTR BoneIdxIdx = 0; BoneIdxIdx < BoneCount; ++BoneIdxIdx)
							{
								const matrix44* pBoneMatrix = ItCurr->pSkinPalette + pModel->BoneIndices[BoneIdxIdx];
								PerInstanceConstValues.SetConstantValue(pConstSkinPalette, BoneIdxIdx, pBoneMatrix, sizeof(matrix44));
							}
						}
						else
						{
							UPTR BoneCount = n_min(ItCurr->BoneCount, pConstSkinPalette->Desc.ElementCount);
							PerInstanceConstValues.SetConstantValue(pConstSkinPalette, 0, ItCurr->pSkinPalette, sizeof(matrix44) * BoneCount);
						}
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

				Sys::DbgOut("CModel rendered non-instanced, tech '%s', group 0x%X, primitives: %d\n", pTech->GetName().CStr(), pGroup, pGroup->IndexCount);
			}
		}

	};

	return ItEnd;
}
//---------------------------------------------------------------------

}