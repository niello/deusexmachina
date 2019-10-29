#include "ModelRenderer.h"

#include <Render/RenderFwd.h>
#include <Render/RenderNode.h>
#include <Render/Model.h>
#include <Render/Material.h>
#include <Render/Effect.h>
#include <Render/ShaderConstant.h>
#include <Render/Mesh.h>
#include <Render/Light.h>
#include <Render/GPUDriver.h>
#include <Math/Sphere.h>	//!!!for light testing only, refactor and optimize!
#include <Core/Factory.h>

namespace Render
{
__ImplementClass(Render::CModelRenderer, 'MDLR', Render::IRenderer);

bool CModelRenderer::Init(bool LightingEnabled)
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
		Cmp.UserDefinedName = nullptr;
		Cmp.Index = i + 4;
		Cmp.Format = VCFmt_Float32_4;
		Cmp.Stream = INSTANCE_BUFFER_STREAM_INDEX;
		Cmp.OffsetInVertex = DEM_VERTEX_COMPONENT_OFFSET_DEFAULT;
		Cmp.PerInstanceData = true;
	}

	//!!!DBG TMP! //???where to define?
	MaxInstanceCount = 30;

	OK;
}
//---------------------------------------------------------------------

bool CModelRenderer::PrepareNode(CRenderNode& Node, const CRenderNodeContext& Context)
{
	CModel* pModel = Node.pRenderable->As<CModel>();
	n_assert_dbg(pModel);

	CMaterial* pMaterial = pModel->Material.Get(); //!!!Get by MaterialLOD!
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

	Node.pMaterial = pMaterial;
	Node.pEffect = pEffect;
	Node.pTech = pEffect->GetTechByInputSet(Node.pSkinPalette ? InputSet_ModelSkinned : InputSet_Model);
	if (!Node.pTech) FAIL;

	Node.pMesh = pModel->Mesh.Get();
	Node.pGroup = pModel->Mesh->GetGroup(pModel->MeshGroupIndex, Context.MeshLOD);

	if (pModel->BoneIndices.GetCount())
	{
		Node.pSkinMapping = pModel->BoneIndices.GetPtr();
		Node.BoneCount = pModel->BoneIndices.GetCount();
	}
	else Node.pSkinMapping = nullptr;

	U8 LightCount = 0;

	if (Context.pLights && MAX_LIGHT_COUNT_PER_OBJECT)
	{
		n_assert_dbg(Context.pLightIndices);

		CArray<U16>& LightIndices = *Context.pLightIndices;
		Node.LightIndexBase = LightIndices.GetCount();

		float LightPriority[MAX_LIGHT_COUNT_PER_OBJECT];
		bool LightOverflow = false;

		const CArray<CLightRecord>& Lights = *Context.pLights;
		for (UPTR i = 0; i < Lights.GetCount(); ++i)
		{
			CLightRecord& LightRec = Lights[i];
			const CLight* pLight = LightRec.pLight;
			switch (pLight->Type)
			{
				case Light_Point:
				{
					//!!!???avoid object creation, rewrite functions so that testing against vector + float is possible!?
					sphere LightBounds(LightRec.Transform.Translation(), pLight->GetRange());
					if (LightBounds.GetClipStatus(Context.AABB) == Outside) continue;
					break;
				}
				case Light_Spot:
				{
					//!!!???PERF: test against sphere before?!
					//???cache GlobalFrustum in a light record?
					matrix44 LocalFrustum;
					pLight->CalcLocalFrustum(LocalFrustum);
					matrix44 GlobalFrustum;
					LightRec.Transform.invert_simple(GlobalFrustum);
					GlobalFrustum *= LocalFrustum;
					if (Context.AABB.GetClipStatus(GlobalFrustum) == Outside) continue;
					break;
				}
			}

			if (LightCount < MAX_LIGHT_COUNT_PER_OBJECT)
			{				
				LightIndices.Add(i);
				++LightCount;
				++LightRec.UseCount;
			}
			else
			{
				//break; // - if don't want to calculate priorities, just skip all remaining lights

				if (!LightOverflow)
				{
					// Calculate light priorities for already collected lights
					for (U16 j = 0; j < LightCount; ++j)
					{
						const CLightRecord& CurrRec = Lights[LightIndices[Node.LightIndexBase + j]];
						LightPriority[j] = pLight->CalcLightPriority(
							Node.Transform.Translation(),
							CurrRec.Transform.Translation(),
							CurrRec.Transform.AxisZ());
					}

					LightOverflow = true;
				}

				float CurrLightPriority = pLight->CalcLightPriority(
					Node.Transform.Translation(),
					LightRec.Transform.Translation(),
					LightRec.Transform.AxisZ());

				//PERF: faster but less precise approach is to replace the first light with lower priority
				float MinPriority = LightPriority[0];
				U16 MinPriorityIndex = 0;
				for (U16 j = 1; j < LightCount; ++j)
				{
					if (LightPriority[j] < MinPriority)
					{
						MinPriority = LightPriority[j];
						MinPriorityIndex = j;
					}
				}
				
				if (CurrLightPriority > MinPriority)
				{
					U16& LightIndex = LightIndices[Node.LightIndexBase + MinPriorityIndex];
					--Lights[LightIndex].UseCount;

					LightIndex = i;
					LightPriority[MinPriorityIndex] = CurrLightPriority;
					++LightRec.UseCount;
				}
			}
		}
	}

	Node.LightCount = LightCount;

	OK;
}
//---------------------------------------------------------------------

// Optimal sorting for the color phase is Tech-Material-Mesh-Group for opaque and then BtF for transparent.
// Tech is sorted before Material because it is more likely that many materials will be rendered with the same
// single-pass tech, than that the same material will be used with many different techs. We have great chances
// to set render state only once as our tech is single-pass, and to render many materials without switching it,
// just rebinding constants, resources and samplers.
CArray<CRenderNode*>::CIterator CModelRenderer::Render(const CRenderContext& Context,
													   CArray<CRenderNode*>& RenderQueue,
													   CArray<CRenderNode*>::CIterator ItCurr)
{
	CGPUDriver& GPU = *Context.pGPU;

	const CMaterial* pCurrMaterial = nullptr;
	const CMesh* pCurrMesh = nullptr;
	const CTechnique* pCurrTech = nullptr;
	CVertexLayout* pVL = nullptr;
	CVertexLayout* pVLInstanced = nullptr;

	// Effect constants
	const CEffectConstant* pConstInstanceDataVS = nullptr;	// Model, ModelSkinned, ModelInstanced
	const CEffectConstant* pConstInstanceDataPS = nullptr;	// Model, ModelSkinned, ModelInstanced
	const CEffectConstant* pConstSkinPalette = nullptr;	// ModelSkinned

	// Subsequent shader constants for single-instance case
	PShaderConstant ConstWorldMatrix;
	PShaderConstant ConstLightCount;
	PShaderConstant ConstLightIndices;

	const bool LightingEnabled = (Context.pLights != nullptr);
	UPTR TechLightCount;

	static const CStrID sidWorldMatrix("WorldMatrix");
	static const CStrID sidLightCount("LightCount");
	static const CStrID sidLightIndices("LightIndices");
	const I32 EMPTY_LIGHT_INDEX = -1;

	CArray<CRenderNode*>::CIterator ItEnd = RenderQueue.End();
	while (ItCurr != ItEnd)
	{
		CRenderNode* pRenderNode = *ItCurr;

		if (pRenderNode->pRenderer != this) return ItCurr;

		const CTechnique* pTech = pRenderNode->pTech;
		const CPrimitiveGroup* pGroup = pRenderNode->pGroup;
		n_assert_dbg(pGroup && pTech);

		// Apply material, if changed

		const CMaterial* pMaterial = pRenderNode->pMaterial;
		if (pMaterial != pCurrMaterial)
		{
			n_assert_dbg(pMaterial);
			n_verify_dbg(pMaterial->Apply(GPU));
			pCurrMaterial = pMaterial;
		}

		// Apply geometry, if changed

		const CMesh* pMesh = pRenderNode->pMesh;
		if (pMesh != pCurrMesh)
		{
			n_assert_dbg(pMesh);
			CVertexBuffer* pVB = pMesh->GetVertexBuffer().Get();
			n_assert_dbg(pVB);
			GPU.SetVertexBuffer(0, pVB);
			GPU.SetIndexBuffer(pMesh->GetIndexBuffer().Get());
			pCurrMesh = pMesh;

			pVL = pVB->GetVertexLayout();
			pVLInstanced = nullptr;
		}

		// Gather instances (no skinned instancing supported)

		UPTR LightCount = pRenderNode->LightCount;

		bool HardwareInstancing = false;
		CArray<CRenderNode*>::CIterator ItInstEnd = ItCurr + 1;
		if (!pRenderNode->pSkinPalette)
		{
			while (ItInstEnd != ItEnd &&
				   (*ItInstEnd)->pRenderer == this &&
				   (*ItInstEnd)->pMaterial == pMaterial &&
				   (*ItInstEnd)->pTech == pTech &&
				   (*ItInstEnd)->pGroup == pGroup &&
				   !(*ItInstEnd)->pSkinPalette)
			{
				// We don't try to find an instanced tech version here, and don't break if
				// it is not found, because if we did, the next object will try to do all
				// this again, not knowing that there is no chance to success. If there is
				// no instanced tech version, we render instances in a loop manually instead.
				const U8 CurrInstLightCount = (*ItInstEnd)->LightCount;
				if (LightCount < CurrInstLightCount) LightCount = CurrInstLightCount;
				++ItInstEnd;
			}

			if (ItInstEnd - ItCurr > 1)
			{
				const CTechnique* pInstancedTech = pRenderNode->pEffect->GetTechByInputSet(InputSet_ModelInstanced);
				if (pInstancedTech)
				{
					pTech = pInstancedTech;
					HardwareInstancing = true;
				}
			}
		}

		// Select tech variation for the current instancing mode and light count

		const CPassList* pPasses = pTech->GetPasses(LightCount);
		n_assert_dbg(pPasses); // To test if it could happen at all
		if (!pPasses)
		{
			ItCurr = ItInstEnd;
			continue;
		}

		// Send lights to GPU if global light buffer is not used

		if (LightingEnabled && !Context.UsesGlobalLightBuffer)
		{
			NOT_IMPLEMENTED;
			if (HardwareInstancing)
			{
				//collect the most used lights for the instance block (or split instances)
				//send these most important lights to GPU (up to max supported by tech)
				//fill pLights' GPULightIndex with batch-local light indices
			}
			else
			{
				//send referenced lights to GPU (up to max supported by tech)
			}
		}

		// Upload per-instance data and draw object(s)

		if (HardwareInstancing)
		{
			if (pTech != pCurrTech)
			{
				pConstInstanceDataVS = pTech->GetConstant(CStrID("InstanceDataVS"));
				pConstInstanceDataPS = pTech->GetConstant(CStrID("InstanceDataPS"));
				pCurrTech = pTech;

				TechLightCount = 0;
				if (LightingEnabled && pConstInstanceDataPS)
				{
					ConstLightIndices = pConstInstanceDataPS->Const->GetElement(0)->GetMember(sidLightIndices);
					if (ConstLightIndices.IsValidPtr())
						TechLightCount = ConstLightIndices->GetElementCount() * ConstLightIndices->GetColumnCount() * ConstLightIndices->GetRowCount();
				}
			}

			if (pConstInstanceDataVS)
			{
				UPTR MaxInstanceCountConst = pConstInstanceDataVS->Const->GetElementCount();
				if (pConstInstanceDataPS)
				{
					const UPTR MaxInstanceCountConstPS = pConstInstanceDataPS->Const->GetElementCount();
					if (MaxInstanceCountConst < MaxInstanceCountConstPS)
						MaxInstanceCountConst = MaxInstanceCountConstPS;
				}
				n_assert_dbg(MaxInstanceCountConst > 1);

				GPU.SetVertexLayout(pVL);

				CConstantBufferSet PerInstanceBuffers;
				PerInstanceBuffers.SetGPU(&GPU);

				CConstantBuffer* pVSCB = PerInstanceBuffers.RequestBuffer(pConstInstanceDataVS->Const->GetConstantBufferHandle(), pConstInstanceDataVS->ShaderType);
				CConstantBuffer* pPSCB = pConstInstanceDataPS ? PerInstanceBuffers.RequestBuffer(pConstInstanceDataPS->Const->GetConstantBufferHandle(), pConstInstanceDataPS->ShaderType) : nullptr;

				UPTR InstanceCount = 0;
				while (ItCurr != ItInstEnd)
				{
					// PERF: can be optimized if necessary (pool allocation, element and member caches, whole buffer construction & commit)
					PShaderConstant CurrInstanceDataVS = pConstInstanceDataVS->Const->GetElement(InstanceCount);

					// Setup instance transformation

					PShaderConstant CurrWorldMatrix = CurrInstanceDataVS->GetMember(sidWorldMatrix);
					if (CurrWorldMatrix.IsValidPtr())
						CurrWorldMatrix->SetMatrix(*pVSCB, &pRenderNode->Transform, 1);

					// Setup instance lights

					if (LightingEnabled && pConstInstanceDataPS && TechLightCount)
					{
						PShaderConstant CurrInstanceDataPS = pConstInstanceDataPS->Const->GetElement(InstanceCount);
						PShaderConstant CurrLightIndices = CurrInstanceDataPS->GetMember(sidLightIndices);

						U32 ActualLightCount;

						if (LightCount == 0)
						{
							// If tech is variable-light-count, set it per instance
							ActualLightCount = (U32)n_min(TechLightCount, pRenderNode->LightCount);
							PShaderConstant CurrLightCount = CurrInstanceDataPS->GetMember(sidLightCount);
							if (CurrLightCount.IsValidPtr())
								CurrLightCount->SetUInt(*pPSCB, ActualLightCount);
						}
						else ActualLightCount = (U32)n_min(LightCount, TechLightCount);

						// Set per-instance light indices
						if (ActualLightCount)
						{
							CArray<U16>::CIterator ItIdx = Context.pLightIndices->IteratorAt(pRenderNode->LightIndexBase);
							U32 InstLightIdx;
							for (InstLightIdx = 0; InstLightIdx < pRenderNode->LightCount; ++InstLightIdx, ++ItIdx)
							{
								if (!Context.UsesGlobalLightBuffer)
								{
									NOT_IMPLEMENTED;
									//!!!???what with batch-local indices?!
								}
								const CLightRecord& LightRec = (*Context.pLights)[(*ItIdx)];
								CurrLightIndices->SetSIntComponent(*pPSCB, InstLightIdx, LightRec.GPULightIndex);
							}

							// If tech is fixed-light-count, fill the first unused light index with the special value
							if (LightCount && InstLightIdx < TechLightCount)
								CurrLightIndices->SetSIntComponent(*pPSCB, InstLightIdx, EMPTY_LIGHT_INDEX);
						}
					}

					++InstanceCount;
					++ItCurr;
					pRenderNode = *ItCurr;

					// If instance buffer is full, render it

					if (InstanceCount == MaxInstanceCountConst)
					{
						PerInstanceBuffers.CommitChanges();
						for (UPTR i = 0; i < pPasses->GetCount(); ++i)
						{
							GPU.SetRenderState((*pPasses)[i]);
							GPU.DrawInstanced(*pGroup, InstanceCount);
						}
						InstanceCount = 0;
						if (ItCurr == ItInstEnd) break;
					}
				}

				// If instance buffer has instances to render, render them

				if (InstanceCount)
				{
					PerInstanceBuffers.CommitChanges();
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

						pVLInstanced = VLInstanced.Get();
						n_assert_dbg(pVLInstanced);
						InstancedLayouts.Add(pVL, VLInstanced);
					}
					else pVLInstanced = InstancedLayouts.ValueAt(VLIdx).Get();
				}

				GPU.SetVertexLayout(pVLInstanced);
				GPU.SetVertexBuffer(INSTANCE_BUFFER_STREAM_INDEX, InstanceVB.Get());

				void* pInstData;
				n_verify(GPU.MapResource(&pInstData, *InstanceVB, Map_WriteDiscard)); //???use big buffer + no overwrite?
				UPTR InstanceCount = 0;
				while (ItCurr != ItInstEnd)
				{
					memcpy(pInstData, pRenderNode->Transform.m, sizeof(matrix44));
					pInstData = (char*)pInstData + sizeof(matrix44);
					++InstanceCount;
					++ItCurr;
					pRenderNode = *ItCurr;

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
		}
		else
		{
			if (pTech != pCurrTech)
			{
				pConstInstanceDataVS = pTech->GetConstant(CStrID("InstanceDataVS"));
				pConstInstanceDataPS = pTech->GetConstant(CStrID("InstanceDataPS"));
				pConstSkinPalette = pTech->GetConstant(CStrID("SkinPalette"));

				// PERF: can be optimized if necessary (pool allocation, element and member caches, whole buffer construction & commit)
				if (pConstInstanceDataVS)
					ConstWorldMatrix = pConstInstanceDataVS->Const->GetMember(sidWorldMatrix);

				TechLightCount = 0;
				if (LightingEnabled && pConstInstanceDataPS)
				{
					ConstLightCount = pConstInstanceDataPS->Const->GetMember(sidLightCount);
					ConstLightIndices = pConstInstanceDataPS->Const->GetMember(sidLightIndices);
					if (ConstLightIndices.IsValidPtr())
						TechLightCount = ConstLightIndices->GetElementCount() * ConstLightIndices->GetColumnCount() * ConstLightIndices->GetRowCount();
				}

				pCurrTech = pTech;
			}

			GPU.SetVertexLayout(pVL);

			for (; ItCurr != ItInstEnd; ++ItCurr, pRenderNode = *ItCurr)
			{
				//???use persistent, create once and store associatively Tech->Values?
				CConstantBufferSet PerInstanceBuffers;
				PerInstanceBuffers.SetGPU(&GPU);

				// Setup per-instance information

				if (pConstInstanceDataVS)
				{
					CConstantBuffer* pVSCB = PerInstanceBuffers.RequestBuffer(pConstInstanceDataVS->Const->GetConstantBufferHandle(), pConstInstanceDataVS->ShaderType);
					
					if (ConstWorldMatrix.IsValidPtr())
						ConstWorldMatrix->SetMatrix(*pVSCB, &pRenderNode->Transform);
				}

				if (pConstInstanceDataPS)
				{
					CConstantBuffer* pPSCB = pConstInstanceDataPS ? PerInstanceBuffers.RequestBuffer(pConstInstanceDataPS->Const->GetConstantBufferHandle(), pConstInstanceDataPS->ShaderType) : nullptr;

					if (LightingEnabled && ConstLightIndices.IsValidPtr())
					{
						U32 ActualLightCount;

						if (LightCount == 0)
						{
							// If tech is variable-light-count, set it per instance
							ActualLightCount = (U32)n_min(TechLightCount, pRenderNode->LightCount);
							if (ConstLightCount.IsValidPtr())
								ConstLightCount->SetUInt(*pPSCB, ActualLightCount);
						}
						else ActualLightCount = (U32)n_min(LightCount, TechLightCount);

						// Set per-instance light indices
						if (ActualLightCount)
						{
							CArray<U16>::CIterator ItIdx = Context.pLightIndices->IteratorAt(pRenderNode->LightIndexBase);
							U32 InstLightIdx;
							for (InstLightIdx = 0; InstLightIdx < pRenderNode->LightCount; ++InstLightIdx, ++ItIdx)
							{
								if (!Context.UsesGlobalLightBuffer)
								{
									NOT_IMPLEMENTED;
									//!!!???what with batch-local indices?!
								}
								const CLightRecord& LightRec = (*Context.pLights)[(*ItIdx)];
								ConstLightIndices->SetSIntComponent(*pPSCB, InstLightIdx, LightRec.GPULightIndex);
							}

							// If tech is fixed-light-count, fill the first unused light index with the special value
							if (LightCount && InstLightIdx < TechLightCount)
								ConstLightIndices->SetSIntComponent(*pPSCB, InstLightIdx, EMPTY_LIGHT_INDEX);
						}
					}
				}

				if (pConstSkinPalette && pRenderNode->pSkinPalette)
				{
					const UPTR BoneCount = n_min(pRenderNode->BoneCount, pConstSkinPalette->Const->GetElementCount());
					CConstantBuffer* pCB = PerInstanceBuffers.RequestBuffer(pConstSkinPalette->Const->GetConstantBufferHandle(), pConstSkinPalette->ShaderType);
					if (pRenderNode->pSkinMapping)
					{
						for (UPTR BoneIdxIdx = 0; BoneIdxIdx < BoneCount; ++BoneIdxIdx)
						{
							const matrix44* pBoneMatrix = pRenderNode->pSkinPalette + pRenderNode->pSkinMapping[BoneIdxIdx];
							pConstSkinPalette->Const->SetMatrix(*pCB, pBoneMatrix, 1, BoneIdxIdx);
						}
					}
					else
					{
						// No mapping, use skin palette directly
						pConstSkinPalette->Const->SetMatrix(*pCB, pRenderNode->pSkinPalette, BoneCount);
					}
				}

				PerInstanceBuffers.CommitChanges();

				// Rendering

				//???loop by pass, then by instance? possibly less render state switches, but possibly more data binding. Does order matter?
				for (UPTR i = 0; i < pPasses->GetCount(); ++i)
				{
					GPU.SetRenderState((*pPasses)[i]);
					GPU.Draw(*pGroup);
				}
			}
		}

	};

	return ItEnd;
}
//---------------------------------------------------------------------

}