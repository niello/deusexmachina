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

static const CStrID sidWorldMatrix("WorldMatrix");
static const CStrID sidLightCount("LightCount");
static const CStrID sidLightIndices("LightIndices");

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
	_pCurrMaterial = nullptr;
	_pCurrMesh = nullptr;
	_pCurrTech = nullptr;
	pVL = nullptr;
	pVLInstanced = nullptr;

	//???!!!can cache for each tech by tech index and don't search constants each frame?
	//could be a material interface for certain input set
	ConstInstanceDataVS = {};
	ConstInstanceDataPS = {};
	ConstSkinPalette = {};
	ConstWorldMatrix = {};
	ConstLightCount = {};
	ConstLightIndices = {};

	OK;
}
//---------------------------------------------------------------------

void CModelRenderer::Render(const CRenderContext& Context, IRenderable& Renderable/*, UPTR SortingKey*/)
{
	CModel& Model = static_cast<CModel&>(Renderable);
	CGPUDriver& GPU = *Context.pGPU;

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

	//!!!if something changes or instance limit has been reached, commit collected instances to rendering here before changing bindings!

	if (pTech != _pCurrTech)
	{
		_pCurrTech = pTech;
		_pCurrMaterial = nullptr;

		//!!!add member offsets for AoS! indexing struct by member will provide its field by very quick O(1). or fill whole structures in C++ and copy to GPU?
		// TODO: could cache all below in a vector/set of structs by Model.ShaderTechIndex or pTech, small amount of items will make search quick
		// New search is needed only here, not per renderable!
		_TechNeedsMaterial = pTech->GetEffect()->GetMaterialParamTable().HasParams();
		//ConstInstanceDataVS = pTech->GetParamTable().GetConstant(CStrID("InstanceDataVS"));
		//ConstInstanceDataPS = pTech->GetParamTable().GetConstant(CStrID("InstanceDataPS"));
		//ConstWorldMatrix = pTech->GetParamTable().GetConstant(CStrID("WorldMatrix"));
		//ConstWorldMatrix = ConstInstanceDataVS ? ConstInstanceDataVS[sidWorldMatrix] : CShaderConstantParam{};
		//ConstLightIndices = ConstInstanceDataPS[0][sidLightIndices];
		//ConstLightIndices = ConstInstanceDataPS.GetMember(sidLightIndices);
		//ConstLightCount = ConstInstanceDataPS.GetMember(sidLightCount);
		//TechLightCount = ConstLightIndices.GetTotalComponentCount();
		//ConstSkinPalette = pTech->GetParamTable().GetConstant(CStrID("SkinPalette"));
		//
		//UPTR MaxInstanceCountConst = ConstInstanceDataVS.GetElementCount();
		//if (ConstInstanceDataPS)
		//{
		//	const UPTR MaxInstanceCountConstPS = ConstInstanceDataPS.GetElementCount();
		//	if (MaxInstanceCountConst < MaxInstanceCountConstPS)
		//		MaxInstanceCountConst = MaxInstanceCountConstPS;
		//}
	}

	if (pMaterial != _pCurrMaterial)
	{
		_pCurrMaterial = pMaterial;
		if (_TechNeedsMaterial) n_verify_dbg(pMaterial->Apply());
	}

	if (pMesh != _pCurrMesh)
	{
		auto pVB = pMesh->GetVertexBuffer().Get();
		GPU.SetVertexLayout(pVB->GetVertexLayout());
		GPU.SetVertexBuffer(0, pVB);
		GPU.SetIndexBuffer(pMesh->GetIndexBuffer().Get());
		_pCurrMesh = pMesh;
	}

	// build per-instance data: world matrix, light indices (if any), skinning palette, animated material params (defaults from material)
	// use different techs for single & instanced or use DrwaIndexedInstanced(1) or use DrawIndexed and hope that SV_InstanceID will be 0. Need testing.

	//???can pack instance world matrix as 4x3 and then unpack in a shader adding 0001?

	//const bool LightingEnabled = (Context.pLights != nullptr);
	//UPTR LightCount = Model.LightCount;
	//const auto& Passes = pTech->GetPasses(LightCount);
	//if (Passes.empty()) return;

	//static_cast<CModel*>(*ItInstEnd)->Material == pMaterial &&
	//static_cast<CModel*>(*ItInstEnd)->ShaderTechIndex == Model.ShaderTechIndex &&
	//static_cast<CModel*>(*ItInstEnd)->pGroup == pGroup &&
	//!static_cast<CModel*>(*ItInstEnd)->pSkinPalette)

	//!!!FIXME: need reusable CShaderParamStorage! Store one per tech in a tech cache structure? or even add one right into the tech for one shot rendering calls, will use tmp buffers.
	//CShaderParamStorage PerInstance(pTech->GetParamTable(), GPU);
	//auto CurrInstanceDataVS = ConstInstanceDataVS[InstanceCount];
	//PerInstance.SetMatrix(CurrInstanceDataVS[sidWorldMatrix], Model.Transform);
	//PerInstance.SetMatrix(ConstWorldMatrix, Model.Transform);
	//if (ConstSkinPalette && Model.pSkinPalette)
	//	PerInstance.SetMatrixArray(ConstSkinPalette, Model.pSkinPalette, std::min(Model.BoneCount, ConstSkinPalette.GetElementCount()));
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

	//PerInstance.Apply();

	//for (const auto& Pass : Passes)
	//{
	//	GPU.SetRenderState(Pass);
	//	GPU.Draw(*pGroup);
	//	GPU.DrawInstanced(*pGroup, InstanceCount);
	//}


	//???need multipass techs or move that to another layer of logic?! multipass tech kills shader sorting and leads to render state switches.
	//but it keeps material and mesh, and maybe even cached intermediate data like GPU skinned vertices buffer.
	//???what effects use multipass techs at all? is there any not implementable with different render phases?
	//may remove pass arrays from tech and leave there only one render state, or at least a set of states for different factor (instance limit etc)

	// recommended: 1 CB for globals, 1 CB for material, 1CB for per-instance data, but random-access in a warp (e.g. skinning better in tbuffer/structured buffer)
	//???use the same obe per-instance buffer for VS and PS? One send, two binds.

	//???send all or only visible lights to GPU? how to detect that something have changed, to avoid resending each frame? set flag when testing
	// lights against frustum and visibility of one actually changes?
	//!!!also don't forget to try filling rendering queues only on renderable object visibility change!
}
//---------------------------------------------------------------------

void CModelRenderer::EndRange(const CRenderContext& Context)
{
}
//---------------------------------------------------------------------

}
