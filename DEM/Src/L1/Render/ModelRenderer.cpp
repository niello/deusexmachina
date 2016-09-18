#include "ModelRenderer.h"

#include <Render/RenderFwd.h>
#include <Render/RenderNode.h>
#include <Render/Model.h>
#include <Render/Material.h>
#include <Render/Effect.h>
#include <Render/ShaderConstant.h>
#include <Render/ConstantBufferSet.h>
#include <Render/Mesh.h>
#include <Render/Light.h>
#include <Render/GPUDriver.h>
#include <Math/Sphere.h>	//!!!for light testing only, refactor and optimize!
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

//???use object pos or AABB and closest point?!
//???!!!precalculate object-invariant values on light cache filling?!
// NB: always returns positive number.
static float CalcLightPriority(const vector3& ObjectPos, const vector3& LightPos, const vector3& LightInvDir, const CLight& Light)
{
	float SqIntensity = Light.Intensity * Light.Intensity;
	if (Light.Type == Light_Directional) return SqIntensity;

	float SqDistance = vector3::SqDistance(ObjectPos, LightPos);
	float Attenuation = (1.f - SqDistance * (Light.GetInvRange() * Light.GetInvRange()));

	if (Light.Type == Light_Spot && SqDistance != 0.f)
	{
		vector3 ModelLight = ObjectPos - LightPos;
		//ModelLight /= n_sqrt(SqDistance);
		ModelLight *= Math::RSqrt(SqDistance); //!!!TEST IT!
		float CosAlpha = ModelLight.Dot(LightInvDir);
		float Falloff = (CosAlpha - Light.GetCosHalfPhi()) / (Light.GetCosHalfTheta() - Light.GetCosHalfPhi());
		return SqIntensity * Attenuation * Clamp(Falloff, 0.f, 1.f);
	}

	return SqIntensity * Attenuation;
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
	Node.pEffect = pEffect;
	Node.pTech = pEffect->GetTechByInputSet(Node.pSkinPalette ? InputSet_ModelSkinned : InputSet_Model);
	if (!Node.pTech) FAIL;

	Node.pMesh = pModel->Mesh.GetUnsafe();
	Node.pGroup = pModel->Mesh->GetGroup(pModel->MeshGroupIndex, Context.MeshLOD);

	if (pModel->BoneIndices.GetCount())
	{
		Node.pSkinMapping = pModel->BoneIndices.GetPtr();
		Node.BoneCount = pModel->BoneIndices.GetCount();
	}
	else Node.pSkinMapping = NULL;

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
					if (!LightBounds.GetClipStatus(Context.AABB)) continue;
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
					if (!Context.AABB.GetClipStatus(GlobalFrustum)) continue;
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
						LightPriority[j] = CalcLightPriority(
							Node.Transform.Translation(),
							CurrRec.Transform.Translation(),
							CurrRec.Transform.AxisZ(),
							*pLight);
					}

					LightOverflow = true;
				}

				float CurrLightPriority = CalcLightPriority(
					Node.Transform.Translation(),
					LightRec.Transform.Translation(),
					LightRec.Transform.AxisZ(),
					*pLight);

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

	const CMaterial* pCurrMaterial = NULL;
	const CMesh* pCurrMesh = NULL;
	const CTechnique* pCurrTech = NULL;
	CVertexLayout* pVL = NULL;
	CVertexLayout* pVLInstanced = NULL;

	// Effect constants
	const CEffectConstant* pConstInstanceData = NULL;	// Model, ModelSkinned, ModelInstanced
	const CEffectConstant* pConstSkinPalette = NULL;	// ModelSkinned

	// Subsequent shader constants for single-instance case
	PShaderConstant ConstWorldMatrix;
	PShaderConstant ConstLightCount;
	PShaderConstant ConstLightIndices;

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
			CVertexBuffer* pVB = pMesh->GetVertexBuffer().GetUnsafe();
			n_assert_dbg(pVB);
			GPU.SetVertexBuffer(0, pVB);
			GPU.SetIndexBuffer(pMesh->GetIndexBuffer().GetUnsafe());
			pCurrMesh = pMesh;

			pVL = pVB->GetVertexLayout();
			pVLInstanced = NULL;
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

		const bool LightingEnabled = (Context.pLights != NULL);
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

		static const CStrID sidWorldMatrix("WorldMatrix");
		static const CStrID sidLightCount("LightCount");
		static const CStrID sidLightIndices("LightIndices");
		const U32 EMPTY_LIGHT_INDEX = (U32)(-1);

		if (HardwareInstancing)
		{
			if (pTech != pCurrTech)
			{
				pConstInstanceData = pTech->GetConstant(CStrID("InstanceDataArray"));
				pCurrTech = pTech;
			}

			if (pConstInstanceData)
			{
				UPTR MaxInstanceCountConst = pConstInstanceData->Const->GetElementCount();
				n_assert_dbg(MaxInstanceCountConst > 1);

				GPU.SetVertexLayout(pVL);

				CConstantBufferSet PerInstanceBuffers;
				PerInstanceBuffers.SetGPU(&GPU);

				CConstantBuffer* pCB = PerInstanceBuffers.RequestBuffer(pConstInstanceData->Const->GetConstantBufferHandle(), pConstInstanceData->ShaderType);

				UPTR InstanceCount = 0;
				while (ItCurr != ItInstEnd)
				{
					// PERF: can be optimized if necessary (pool allocation, element and member caches, whole buffer construction & commit)
					PShaderConstant CurrInstanceData = pConstInstanceData->Const->GetElement(InstanceCount);

					// Setup instance transformation

					PShaderConstant CurrWorldMatrix = CurrInstanceData->GetMember(sidWorldMatrix);
					if (CurrWorldMatrix.IsValidPtr())
						CurrWorldMatrix->SetMatrix(*pCB, &pRenderNode->Transform, 1);

					// Setup instance lights

					if (LightingEnabled)
					{
						PShaderConstant CurrLightIndices = CurrInstanceData->GetMember(sidLightIndices);
						if (CurrLightIndices.IsValidPtr())
						{
							//!!!mul elm count on columns!
							UPTR TechLightCount = CurrLightIndices->GetElementCount(); //!!!const, may be obtained from shader metadata outside the loop!
							U32 ActualLightCount;

							if (LightCount == 0)
							{
								// If tech is variable-light-count, set it per instance
								ActualLightCount = (U32)n_min(TechLightCount, pRenderNode->LightCount);
								PShaderConstant CurrLightCount = CurrInstanceData->GetMember(sidLightCount);
								if (CurrLightCount.IsValidPtr())
									CurrLightCount->SetUInt(*pCB, ActualLightCount);
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
									CurrLightIndices->SetUInt(*pCB, LightRec.GPULightIndex);
								}
								if (LightCount)
								{
									// If tech is fixed-light-count, fill unused light indices with the special value
									for (; InstLightIdx < TechLightCount; ++InstLightIdx)
										CurrLightIndices->SetUInt(*pCB, EMPTY_LIGHT_INDEX);
								}
							}
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
				pConstInstanceData = pTech->GetConstant(CStrID("InstanceData"));
				pConstSkinPalette = pTech->GetConstant(CStrID("SkinPalette"));

				// PERF: can be optimized if necessary (pool allocation, element and member caches, whole buffer construction & commit)
				if (pConstInstanceData)
				{
					ConstWorldMatrix = pConstInstanceData->Const->GetMember(sidWorldMatrix);
					if (LightingEnabled)
					{
						ConstLightCount = pConstInstanceData->Const->GetMember(sidLightCount);
						ConstLightIndices = pConstInstanceData->Const->GetMember(sidLightIndices);
					}
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

				if (pConstInstanceData)
				{
					CConstantBuffer* pCB = PerInstanceBuffers.RequestBuffer(pConstInstanceData->Const->GetConstantBufferHandle(), pConstInstanceData->ShaderType);

					if (ConstWorldMatrix.IsValidPtr())
						ConstWorldMatrix->SetMatrix(*pCB, &pRenderNode->Transform);

					if (LightingEnabled)
					{
						if (ConstLightIndices.IsValidPtr())
						{
							//!!!mul elm count on columns!
							UPTR TechLightCount = ConstLightIndices->GetElementCount(); //!!!const, may be obtained from shader metadata outside the loop!
							U32 ActualLightCount;

							if (LightCount == 0)
							{
								// If tech is variable-light-count, set it per instance
								ActualLightCount = (U32)n_min(TechLightCount, pRenderNode->LightCount);
								if (ConstLightCount.IsValidPtr())
									ConstLightCount->SetUInt(*pCB, ActualLightCount);
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
									ConstLightIndices->SetUInt(*pCB, LightRec.GPULightIndex);
								}
								if (LightCount)
								{
									// If tech is fixed-light-count, fill unused light indices with the special value
									for (; InstLightIdx < TechLightCount; ++InstLightIdx)
										ConstLightIndices->SetUInt(*pCB, EMPTY_LIGHT_INDEX);
								}
							}
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