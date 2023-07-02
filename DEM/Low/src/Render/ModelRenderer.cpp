#include "ModelRenderer.h"

#include <Render/RenderFwd.h>
#include <Render/RenderNode.h>
#include <Render/Model.h>
#include <Render/Material.h>
#include <Render/Effect.h>
#include <Render/Mesh.h>
#include <Render/Light.h>
#include <Render/GPUDriver.h>
#include <Math/Sphere.h>	//!!!for light testing only, refactor and optimize!
#include <Data/Params.h>
#include <Core/Factory.h>

namespace Render
{
FACTORY_CLASS_IMPL(Render::CModelRenderer, 'MDLR', Render::IRenderer);

bool CModelRenderer::Init(bool LightingEnabled, const Data::CParams& Params)
{
	InstanceDataDecl.SetSize(4);

	// World matrix
	for (U32 i = 0; i < 4; ++i)
	{
		CVertexComponent& Cmp = InstanceDataDecl[i];
		Cmp.Semantic = EVertexComponentSemantic::TexCoord;
		Cmp.UserDefinedName = nullptr;
		Cmp.Index = i + 4;
		Cmp.Format = EVertexComponentFormat::Float32_4;
		Cmp.Stream = INSTANCE_BUFFER_STREAM_INDEX;
		Cmp.OffsetInVertex = VertexComponentOffsetAuto;
		Cmp.PerInstanceData = true;
	}

	InstanceVBSize = std::max(0, Params.Get<int>(CStrID("InstanceVBSize"), 30));

	OK;
}
//---------------------------------------------------------------------

bool CModelRenderer::PrepareNode(IRenderable& Node, const CRenderNodeContext& Context)
{
	CModel* pModel = Node.As<CModel>();
	n_assert_dbg(pModel);

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
			const CLight_OLD_DELETE* pLight = LightRec.pLight;
			switch (pLight->Type)
			{
				case Light_Point:
				{
					//!!!???avoid object creation, rewrite functions so that testing against vector + float is possible!?
					sphere LightBounds(LightRec.Transform.Translation(), pLight->GetRange());
					if (LightBounds.GetClipStatus(Context.AABB) == EClipStatus::Outside) continue;
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
					if (Context.AABB.GetClipStatus(GlobalFrustum) == EClipStatus::Outside) continue;
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
CRenderQueueIterator CModelRenderer::Render(const CRenderContext& Context, CRenderQueue& RenderQueue, CRenderQueueIterator ItCurr)
{
	CGPUDriver& GPU = *Context.pGPU;

	const CMaterial* pCurrMaterial = nullptr;
	const CMesh* pCurrMesh = nullptr;
	const CTechnique* pCurrTech = nullptr;
	CVertexLayout* pVL = nullptr;
	CVertexLayout* pVLInstanced = nullptr;

	// Effect constants
	CShaderConstantParam ConstInstanceDataVS; // Model, ModelSkinned, ModelInstanced
	CShaderConstantParam ConstInstanceDataPS; // Model, ModelSkinned, ModelInstanced
	CShaderConstantParam ConstSkinPalette;    // ModelSkinned

	// Subsequent shader constants for single-instance case
	CShaderConstantParam ConstWorldMatrix;
	CShaderConstantParam ConstLightCount;
	CShaderConstantParam ConstLightIndices;

	const bool LightingEnabled = (Context.pLights != nullptr);
	UPTR TechLightCount;

	static const CStrID sidWorldMatrix("WorldMatrix");
	static const CStrID sidLightCount("LightCount");
	static const CStrID sidLightIndices("LightIndices");
	const I32 EMPTY_LIGHT_INDEX = -1;

	CRenderQueueIterator ItEnd = RenderQueue.End();
	while (ItCurr != ItEnd)
	{
		IRenderable* pRenderNode = *ItCurr;

		if (pRenderNode->pRenderer != this) return ItCurr;

		CModel* pModel = pRenderNode->As<CModel>();
		n_assert_dbg(pModel);

		const CTechnique* pTech = Context.pShaderTechCache[pModel->ShaderTechIndex];
		const CPrimitiveGroup* pGroup = pModel->pGroup;
		n_assert_dbg(pGroup && pTech);

		// Apply material, if changed

		auto pMaterial = pModel->Material.Get();
		if (pMaterial != pCurrMaterial)
		{
			n_assert_dbg(pMaterial);
			n_verify_dbg(pMaterial->Apply());
			pCurrMaterial = pMaterial;
		}

		// Apply geometry, if changed

		const CMesh* pMesh = pModel->Mesh.Get();
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
		CRenderQueueIterator ItInstEnd = ItCurr + 1;
		if (!pModel->pSkinPalette && (ConstInstanceDataVS || InstanceVBSize > 1))
		{
			while (ItInstEnd != ItEnd &&
				(*ItInstEnd)->IsA<CModel>() &&
				   static_cast<CModel*>(*ItInstEnd)->pRenderer == this &&
				   static_cast<CModel*>(*ItInstEnd)->Material == pMaterial &&
				   static_cast<CModel*>(*ItInstEnd)->ShaderTechIndex == pModel->ShaderTechIndex &&
				   static_cast<CModel*>(*ItInstEnd)->pGroup == pGroup &&
				   !static_cast<CModel*>(*ItInstEnd)->pSkinPalette)
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
				static const CStrID InputSet_ModelInstanced("ModelInstanced");
				if (const auto pInstancedTech = pTech->GetEffect()->GetTechByInputSet(InputSet_ModelInstanced))
				{
					pTech = pInstancedTech;
					HardwareInstancing = true;
				}
			}
		}

		// Select tech variation for the current instancing mode and light count

		const auto& Passes = pTech->GetPasses(LightCount);
		if (Passes.empty())
		{
			ItCurr = ItInstEnd;
			continue;
		}

		// Send lights to GPU if global light buffer is not used

		if (LightingEnabled && LightCount && !Context.UsesGlobalLightBuffer)
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
				ConstInstanceDataVS = pTech->GetParamTable().GetConstant(CStrID("InstanceDataVS"));
				ConstInstanceDataPS = pTech->GetParamTable().GetConstant(CStrID("InstanceDataPS"));
				pCurrTech = pTech;

				TechLightCount = 0;
				if (LightingEnabled && ConstInstanceDataPS)
				{
					ConstLightIndices = ConstInstanceDataPS[0][sidLightIndices];
					TechLightCount = ConstLightIndices.GetTotalComponentCount();
				}
			}

			if (ConstInstanceDataVS)
			{
				UPTR MaxInstanceCountConst = ConstInstanceDataVS.GetElementCount();
				if (ConstInstanceDataPS)
				{
					const UPTR MaxInstanceCountConstPS = ConstInstanceDataPS.GetElementCount();
					if (MaxInstanceCountConst < MaxInstanceCountConstPS)
						MaxInstanceCountConst = MaxInstanceCountConstPS;
				}
				n_assert_dbg(MaxInstanceCountConst > 1);

				GPU.SetVertexLayout(pVL);

				CShaderParamStorage PerInstance(pTech->GetParamTable(), GPU);

				UPTR InstanceCount = 0;
				while (ItCurr != ItInstEnd)
				{
					auto CurrInstanceDataVS = ConstInstanceDataVS[InstanceCount];

					// Setup instance transformation

					PerInstance.SetMatrix(CurrInstanceDataVS[sidWorldMatrix], pRenderNode->Transform);

					// Setup instance lights

					if (TechLightCount)
					{
						CShaderConstantParam CurrInstanceDataPS = ConstInstanceDataPS.GetElement(InstanceCount);
						CShaderConstantParam CurrLightIndices = CurrInstanceDataPS[sidLightIndices];

						U32 ActualLightCount;

						if (LightCount == 0)
						{
							// If tech is variable-light-count, set it per instance
							ActualLightCount = std::min(TechLightCount, static_cast<UPTR>(pRenderNode->LightCount));
							PerInstance.SetUInt(CurrInstanceDataPS[sidLightCount], ActualLightCount);
						}
						else ActualLightCount = std::min(LightCount, TechLightCount);

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
								PerInstance.SetInt(CurrLightIndices.GetComponent(InstLightIdx), LightRec.GPULightIndex);
							}

							// If tech is fixed-light-count, fill the first unused light index with the special value
							if (LightCount && InstLightIdx < TechLightCount)
								PerInstance.SetInt(CurrLightIndices.GetComponent(InstLightIdx), EMPTY_LIGHT_INDEX);
						}
					}

					++InstanceCount;
					++ItCurr;
					pRenderNode = *ItCurr;

					// If instance buffer is full, render it

					if (InstanceCount == MaxInstanceCountConst)
					{
						PerInstance.Apply();
						for (const auto& Pass : Passes)
						{
							GPU.SetRenderState(Pass);
							GPU.DrawInstanced(*pGroup, InstanceCount);
						}
						InstanceCount = 0;
						if (ItCurr == ItInstEnd) break;
					}
				}

				// If instance buffer has instances to render, render them

				if (InstanceCount)
				{
					PerInstance.Apply();
					for (const auto& Pass : Passes)
					{
						GPU.SetRenderState(Pass);
						GPU.DrawInstanced(*pGroup, InstanceCount);
					}
				}
			}
			else
			{
				n_assert_dbg(InstanceVBSize > 1);

				// We create this buffer lazy because for D3D11 possibility is high to use only constant-based instancing
				if (InstanceVB.IsNullPtr())
				{
					PVertexLayout VLInstanceData = GPU.CreateVertexLayout(InstanceDataDecl.data(), InstanceDataDecl.size());
					InstanceVB = GPU.CreateVertexBuffer(*VLInstanceData, InstanceVBSize, Access_CPU_Write | Access_GPU_Read);
				}

				if (!pVLInstanced)
				{
					auto It = InstancedLayouts.find(pVL);
					if (It == InstancedLayouts.cend())
					{
						constexpr UPTR MAX_COMPONENTS = 64;

						UPTR BaseComponentCount = pVL->GetComponentCount();
						UPTR InstComponentCount = InstanceDataDecl.size();
						UPTR DescComponentCount = BaseComponentCount + InstComponentCount;

						if (DescComponentCount > MAX_COMPONENTS)
						{
							::Sys::Error("CModelRenderer::Render() > too many vertex layout components");
							BaseComponentCount = std::min(BaseComponentCount, MAX_COMPONENTS);
							DescComponentCount = std::min(DescComponentCount, MAX_COMPONENTS);
							InstComponentCount = DescComponentCount - BaseComponentCount;
						}

						CVertexComponent InstancedDecl[MAX_COMPONENTS];
						memcpy(InstancedDecl, pVL->GetComponent(0), BaseComponentCount * sizeof(CVertexComponent));
						memcpy(InstancedDecl + BaseComponentCount, InstanceDataDecl.data(), InstComponentCount * sizeof(CVertexComponent));

						PVertexLayout VLInstanced = GPU.CreateVertexLayout(InstancedDecl, DescComponentCount);

						pVLInstanced = VLInstanced.Get();
						InstancedLayouts.emplace(pVL, VLInstanced);
					}
					else pVLInstanced = It->second.Get();
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

					if (InstanceCount == InstanceVBSize)
					{
						GPU.UnmapResource(*InstanceVB);
						for (const auto& Pass : Passes)
						{
							GPU.SetRenderState(Pass);
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
					for (const auto& Pass : Passes)
					{
						GPU.SetRenderState(Pass);
						GPU.DrawInstanced(*pGroup, InstanceCount);
					}
				}
			}
		}
		else
		{
			if (pTech != pCurrTech)
			{
				ConstInstanceDataVS = pTech->GetParamTable().GetConstant(CStrID("InstanceDataVS"));
				ConstInstanceDataPS = pTech->GetParamTable().GetConstant(CStrID("InstanceDataPS"));
				ConstSkinPalette = pTech->GetParamTable().GetConstant(CStrID("SkinPalette"));

				// PERF: can be optimized if necessary (pool allocation, element and member caches, whole buffer construction & commit)
				ConstWorldMatrix = ConstInstanceDataVS ? ConstInstanceDataVS[sidWorldMatrix] : CShaderConstantParam{};

				TechLightCount = 0;
				if (LightingEnabled && ConstInstanceDataPS)
				{
					ConstLightCount = ConstInstanceDataPS.GetMember(sidLightCount);
					ConstLightIndices = ConstInstanceDataPS.GetMember(sidLightIndices);
					TechLightCount = ConstLightIndices.GetTotalComponentCount();
				}

				pCurrTech = pTech;
			}

			GPU.SetVertexLayout(pVL);

			for (; ItCurr != ItInstEnd; ++ItCurr, pRenderNode = *ItCurr)
			{
				//???use persistent, create once and store associatively Tech->Values?
				CShaderParamStorage PerInstance(pTech->GetParamTable(), GPU);

				// Setup per-instance information

				PerInstance.SetMatrix(ConstWorldMatrix, pRenderNode->Transform);

				if (TechLightCount)
				{
					U32 ActualLightCount;

					if (LightCount == 0)
					{
						// If tech is variable-light-count, set it per instance
						ActualLightCount = std::min(TechLightCount, static_cast<UPTR>(pRenderNode->LightCount));
						PerInstance.SetUInt(ConstLightCount, ActualLightCount);
					}
					else ActualLightCount = std::min(LightCount, TechLightCount);

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
							PerInstance.SetInt(ConstLightIndices.GetComponent(InstLightIdx), LightRec.GPULightIndex);
						}

						// If tech is fixed-light-count, fill the first unused light index with the special value
						if (LightCount && InstLightIdx < TechLightCount)
							PerInstance.SetInt(ConstLightIndices.GetComponent(InstLightIdx), EMPTY_LIGHT_INDEX);
					}
				}

				// TODO: could reuse constant buffer with the same pSkinPalette without resetting the palette redundantly
				if (ConstSkinPalette && pModel->pSkinPalette)
					PerInstance.SetMatrixArray(ConstSkinPalette, pModel->pSkinPalette, std::min(pModel->BoneCount, ConstSkinPalette.GetElementCount()));

				PerInstance.Apply();

				// Rendering

				//???loop by pass, then by instance? possibly less render state switches, but possibly more data binding. Does order matter?
				for (const auto& Pass : Passes)
				{
					GPU.SetRenderState(Pass);
					GPU.Draw(*pGroup);
				}
			}
		}

	};

	return ItEnd;
}
//---------------------------------------------------------------------

}
