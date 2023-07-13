#include "ModelRenderer.h"

#include <Render/RenderFwd.h>
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

static const CStrID sidInstanceData("InstanceData");
static const CStrID sidSkinPalette("SkinPalette");
static const CStrID sidWorldMatrix("WorldMatrix");
static const CStrID sidFirstBoneIndex("FirstBoneIndex");
static const CStrID sidLightIndices("LightIndices");
static const CStrID sidLightCount("LightCount");

bool CModelRenderer::Init(bool LightingEnabled, const Data::CParams& Params)
{
	OK;
}
//---------------------------------------------------------------------

bool CModelRenderer::PrepareNode(IRenderable& Node, const CRenderNodeContext& Context)
{
	CModel* pModel = Node.As<CModel>();
	n_assert_dbg(pModel);

	U8 LightCount = 0;

	/*
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
	*/

	Node.LightCount = LightCount;

	OK;
}
//---------------------------------------------------------------------

bool CModelRenderer::BeginRange(const CRenderContext& Context)
{
	_pCurrTech = nullptr;
	_pCurrMaterial = nullptr;
	_pCurrMesh = nullptr;
	_pCurrGroup = nullptr;
	_InstanceCount = 0;
	_TechMaxInstanceCount = 1;
	_TechNeedsMaterial = false;

	OK;
}
//---------------------------------------------------------------------

void CModelRenderer::Render(const CRenderContext& Context, IRenderable& Renderable/*, UPTR SortingKey*/)
{
	CModel& Model = static_cast<CModel&>(Renderable);
	CGPUDriver& GPU = *Context.pGPU;
	const bool LightingEnabled = (Context.pLights != nullptr);

	const CTechnique* pTech = Context.pShaderTechCache[Model.ShaderTechIndex];
	n_assert_dbg(pTech);
	if (!pTech) return;

	auto pMaterial = Model.Material.Get();
	n_assert_dbg(pMaterial);
	if (!pMaterial) return;

	const CMesh* pMesh = Model.Mesh.Get();
	n_assert_dbg(pMesh);
	if (!pMesh) return;

	const CPrimitiveGroup* pGroup = Model.pGroup;
	n_assert_dbg(pGroup);
	if (!pGroup) return;

	// Detect batch breaking and commit collected instances to GPU

	if (_InstanceCount == _TechMaxInstanceCount) CommitCollectedInstances();

	if (pTech != _pCurrTech)
	{
		if (_InstanceCount) CommitCollectedInstances();

		_pCurrTech = pTech;

		//!!!add member offsets for AoS! indexing struct by member will provide its field by very quick O(1). or fill whole structures in C++ and copy to GPU?
		//instead of member offset in struct, can make member array based on offsets, and assign this like AoS: WorldMatrix[i] = mtx. Array with custom stride?
		// TODO: could cache all below in a vector/set of structs by Model.ShaderTechIndex or pTech, small amount of items will make search quick
		// New search is needed only here, not per renderable!
		_TechNeedsMaterial = pTech->GetEffect()->GetMaterialParamTable().HasParams();

		_ConstInstanceData = pTech->GetParamTable().GetConstant(sidInstanceData);
		_ConstSkinPalette = pTech->GetParamTable().GetConstant(sidSkinPalette);
		if (_ConstSkinPalette)
		{
			_MemberFirstBoneIndex = _ConstInstanceData[0][sidFirstBoneIndex];
		}
		else
		{
			_MemberWorldMatrix = _ConstInstanceData[0][sidWorldMatrix];
		}

		if (LightingEnabled)
		{
			// ... try to find per-instance light shader constants ...
		}

		CalculateMaxInstanceCount();

		// Verify that tech is selected correctly. Can remove later if never asserts. It shouldn't.
		n_assert_dbg(static_cast<bool>(Model.BoneCount) == static_cast<bool>(_ConstSkinPalette));
	}

	//!!!FIXME: now need to commit if skinning buffer remaining count is less than current object bone count!
	//!!!Handle case when whole buffer is less than object's skin palette! BooneCount = min(Model.BoneCount, tech's buffer size), but need tech first!!!
	//!!!set _CurrBoneCount = 0 when commit or only when wrap the buffer?!
	//if (Model.BoneCount != _CurrBoneCount)
	//{
	//	if (_InstanceCount) CommitCollectedInstances();

	//	_CurrBoneCount = Model.BoneCount;
	//	CalculateMaxInstanceCount();
	//}

	UPTR LightCount = Model.LightCount;
	const auto& Passes = pTech->GetPasses(LightCount);
	if (Passes.empty()) return;

	if (_TechNeedsMaterial && pMaterial != _pCurrMaterial)
	{
		if (_InstanceCount) CommitCollectedInstances();

		_pCurrMaterial = pMaterial;
		n_verify_dbg(pMaterial->Apply());
	}

	if (pMesh != _pCurrMesh)
	{
		if (_InstanceCount) CommitCollectedInstances();

		auto pVB = pMesh->GetVertexBuffer().Get();
		GPU.SetVertexLayout(pVB->GetVertexLayout());
		GPU.SetVertexBuffer(0, pVB);
		GPU.SetIndexBuffer(pMesh->GetIndexBuffer().Get());
		_pCurrMesh = pMesh;
	}

	if (pGroup != _pCurrGroup)
	{
		if (_InstanceCount) CommitCollectedInstances();

		_pCurrGroup = pGroup;
	}

	// Setup per-instance data

	//!!!FIXME: need reusable CShaderParamStorage! Store one per tech in a tech cache structure? or even add one right into the tech for one shot rendering calls, will use tmp buffers.
	CShaderParamStorage PerInstance(pTech->GetParamTable(), GPU);
	if (_MemberWorldMatrix)
	{
		_MemberWorldMatrix.Shift(_ConstInstanceData, _InstanceCount);
		PerInstance.SetMatrix(_MemberWorldMatrix, Model.Transform);
	}

	if (Model.pSkinPalette)
	{
		_MemberFirstBoneIndex.Shift(_ConstInstanceData, _InstanceCount);
		PerInstance.SetUInt(_MemberWorldMatrix, _CurrBoneCount);

		//!!!this allows using _ConstSkinPalette curcularly with no_overwrite! if out of space, wrap and discard and start filling from beginning. Hide inside CShaderParamStorage?
		//!!!make sure that only a part of the buffer is updated and submitted to GPU!
		const auto InstanceBoneCount = std::min(Model.BoneCount, _ConstSkinPalette.GetElementCount() - _CurrBoneCount);
		PerInstance.SetMatrixArray(_ConstSkinPalette, Model.pSkinPalette, InstanceBoneCount, _CurrBoneCount);
		_CurrBoneCount += InstanceBoneCount;
	}

	++_InstanceCount;

	//!!!DBG TMP!
	PerInstance.Apply();
	for (const auto& Pass : Passes)
	{
		GPU.SetRenderState(Pass);
		if (_InstanceCount > 1)
			GPU.DrawInstanced(*pGroup, _InstanceCount);
		else
			GPU.Draw(*pGroup); //!!!TODO PERF: check if this is better for a single object! DrawInstanced(1) works either! Maybe there is no profit in branching here!
	}
	_InstanceCount = 0;
	//////////////

	// build per-instance data: world matrix, light indices (if any), skinning palette, animated material params (defaults from material)
	// use different techs for single & instanced or use DrwaIndexedInstanced(1) or use DrawIndexed and hope that SV_InstanceID will be 0. Need testing.

	//???can pack instance world matrix as 4x3 and then unpack in a shader adding 0001?


	//static_cast<CModel*>(*ItInstEnd)->Material == pMaterial &&
	//static_cast<CModel*>(*ItInstEnd)->ShaderTechIndex == Model.ShaderTechIndex &&
	//static_cast<CModel*>(*ItInstEnd)->pGroup == pGroup &&
	//!static_cast<CModel*>(*ItInstEnd)->pSkinPalette)

	//CShaderConstantParam CurrInstanceDataPS = ConstInstanceDataPS.GetElement(InstanceCount);
	//CShaderConstantParam CurrLightIndices = CurrInstanceDataPS[sidLightIndices];
	//if (TechLightCount)
	//{
	//	U32 ActualLightCount;
	//	if (LightCount == 0)
	//	{
	//		// If tech is variable-light-count, set it per instance
	//		ActualLightCount = std::min(TechLightCount, static_cast<UPTR>(Model.LightCount));
	//		PerInstance.SetUInt(CurrInstanceDataPS[sidLightCount], ActualLightCount);
	//	}
	//	else ActualLightCount = std::min(LightCount, TechLightCount);

	//	// Set per-instance light indices
	//	if (ActualLightCount)
	//	{
	//		CArray<U16>::CIterator ItIdx = Context.pLightIndices->IteratorAt(Model.LightIndexBase);
	//		U32 InstLightIdx;
	//		for (InstLightIdx = 0; InstLightIdx < Model.LightCount; ++InstLightIdx, ++ItIdx)
	//		{
	//			const CLightRecord& LightRec = (*Context.pLights)[(*ItIdx)];
	//			PerInstance.SetInt(CurrLightIndices.GetComponent(InstLightIdx), LightRec.GPULightIndex);
	//		}

	//		// If tech is fixed-light-count, fill the first unused light index with the special value
	//		if (LightCount && InstLightIdx < TechLightCount)
	//			PerInstance.SetInt(CurrLightIndices.GetComponent(InstLightIdx), EMPTY_LIGHT_INDEX);
	//	}
	//}


	//???need multipass techs or move that to another layer of logic?! multipass tech kills shader sorting and leads to render state switches.
	//but it keeps material and mesh, and maybe even cached intermediate data like GPU skinned vertices buffer.
	//???what effects use multipass techs at all? is there any not implementable with different render phases?
	//may remove pass arrays from tech and leave there only one render state, or at least a set of states for different factor (instance limit etc)

	// recommended: 1 CB for globals, 1 CB for material, 1CB for per-instance data, but random-access in a warp (e.g. skinning better in tbuffer/structured buffer)
	//???use the same obe per-instance buffer for VS and PS? One send, two binds.
	//!!!Sharing constant buffers between shaders (binding the same CB to the VS and PS) also can improve performance. (c)

	//!!!when skinning, skin position and normal in the same loop! Need only one fetch per bone matrix then, and less control instructions! or check optimization!
	//!!!use float4x3 for skinning! and for world matrix too?
	//???tbuffer or StructuredBuffer for bones? the second can change its size in runtime?
	//for skinned instancing can pass bone count to shader and index as BoneCount*InstanceID+i! Instance count will be calculated from shader consts!

	//???send all or only visible lights to GPU? how to detect that something have changed, to avoid resending each frame? set flag when testing
	// lights against frustum and visibility of one actually changes?
	//!!!also don't forget to try filling rendering queues only on renderable object visibility change!

	//!!!updating big CB loads the bus with unnecessary bytes. Using big per-instance data array for few instances will pass too many unnecessary traffic.
	//what about UpdateSubresource? Can it make this better? Could exploit no-overwrite and offsets to avoid stalls drawing from previous region!
	//???also could use a pool of buffers of different sizes, and bind shorter buffer than shader expects, guaranteeing that extra data is not accessed?
	//Q: What is the best way to update constant buffers?
	//A: UpdateSubresource and Map with Discard should be about the same speed. Choose between them depending on which one copies the least amount of memory.
	//   If you already have your data stored in memory in one contiguous block, use UpdateSubresource.If you need to accumulate data from other places, use Map with Discard.

	//???calc skinned normal with float3x3 or as vOutput.vNor += mul( float4(vInput.vNor, 0.0f), amPalette[ aiIndices[ iBone  ] ] ) * fWeight; ?

	//!!!skin palettes can be pre-uploaded in z prepass and reused in color pass! But may not work well with instancing.
	//Q: How much can I improve my frame rate if I only upload my character's bones once per frame instead of once per pass/draw?
	//A: You can improve frame rate between 8 percent and 50 percent depending on the amount of redundant data.In the worst case, performance will not be reduced.
	//https://learn.microsoft.com/en-us/windows/win32/dxtecharts/direct3d10-frequently-asked-questions
}
//---------------------------------------------------------------------

void CModelRenderer::EndRange(const CRenderContext& Context)
{
	if (_InstanceCount) CommitCollectedInstances();
}
//---------------------------------------------------------------------

void CModelRenderer::CalculateMaxInstanceCount()
{
	_TechMaxInstanceCount = _ConstInstanceData.GetElementCount();
	if (_ConstSkinPalette && _CurrBoneCount)
	{
		const UPTR MaxSkinInstanceCount = _ConstSkinPalette.GetElementCount() / _CurrBoneCount;
		if (_TechMaxInstanceCount > MaxSkinInstanceCount)
			_TechMaxInstanceCount = MaxSkinInstanceCount;
	}
	if (!_TechMaxInstanceCount) _TechMaxInstanceCount = 1;
}
//---------------------------------------------------------------------

void CModelRenderer::CommitCollectedInstances()
{
	//!!!PERF: needs testing on big scene. Check is moved outside the call to reduce redundant call count, but is it necessary?
	n_assert_dbg(_InstanceCount);

	//PerInstance.Apply();

	//for (const auto& Pass : Passes)
	//{
	//	GPU.SetRenderState(Pass);
	//	GPU.Draw(*pGroup);
	//	GPU.DrawInstanced(*pGroup, InstanceCount);
	//}
}
//---------------------------------------------------------------------

}
