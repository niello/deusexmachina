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
	_pGPU = nullptr;
	_InstanceCount = 0;
	_TechMaxInstanceCount = 1;
	_TechNeedsMaterial = false;

	OK;
}
//---------------------------------------------------------------------

// For constant buffer handling see https://learn.microsoft.com/en-us/windows/win32/dxtecharts/direct3d10-frequently-asked-questions
void CModelRenderer::Render(const CRenderContext& Context, IRenderable& Renderable/*, UPTR SortingKey*/)
{
	CModel& Model = static_cast<CModel&>(Renderable);
	const bool LightingEnabled = (Context.pLights != nullptr);

	_pGPU = Context.pGPU;

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

	// Detect batch breaking, commit collected instances to GPU and prepare the new batch

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
		_MemberFirstBoneIndex = _ConstInstanceData[0][sidFirstBoneIndex];
		_MemberWorldMatrix = _ConstInstanceData[0][sidWorldMatrix];

		if (LightingEnabled)
		{
			_MemberLightCount = _ConstInstanceData[0][sidLightCount];
			_MemberLightIndices = _ConstInstanceData[0][sidLightIndices];
		}

		_PerInstance = CShaderParamStorage(pTech->GetParamTable(), *_pGPU); // TODO: store in a tech cache too!

		_TechMaxInstanceCount = _ConstInstanceData ? _ConstInstanceData.GetElementCount() : std::numeric_limits<U32>::max();

		// Verify that tech is selected correctly. Can remove later if never asserts. It shouldn't.
		n_assert_dbg(static_cast<bool>(Model.BoneCount) == static_cast<bool>(_ConstSkinPalette));
	}
	else
	{
		// Check if the limit of per-instance data for the tech is reached. Commit if either:
		// - constant buffer with instance data structures is filled
		// - the model is skinned and the tech doesn't support skinned instancing
		// - model's bones do not fit into the remaining part of the shader skin palette buffer which is already not empty
		if (_InstanceCount == _TechMaxInstanceCount ||
			(_InstanceCount && Model.BoneCount && !_MemberFirstBoneIndex) ||
			(_BufferedBoneCount && Model.BoneCount > _ConstSkinPalette.GetElementCount() - _BufferedBoneCount))
		{
			CommitCollectedInstances();
		}
	}

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
		_pGPU->SetVertexLayout(pVB->GetVertexLayout());
		_pGPU->SetVertexBuffer(0, pVB);
		_pGPU->SetIndexBuffer(pMesh->GetIndexBuffer().Get());
		_pCurrMesh = pMesh;
	}

	if (pGroup != _pCurrGroup)
	{
		if (_InstanceCount) CommitCollectedInstances();

		_pCurrGroup = pGroup;
	}

	// Setup per-instance data

	if (_MemberWorldMatrix)
	{
		_MemberWorldMatrix.Shift(_ConstInstanceData, _InstanceCount);
		_PerInstance.SetMatrix(_MemberWorldMatrix, Model.Transform);
	}

	if (Model.pSkinPalette)
	{
		if (_MemberFirstBoneIndex)
		{
			_MemberFirstBoneIndex.Shift(_ConstInstanceData, _InstanceCount);
			_PerInstance.SetUInt(_MemberFirstBoneIndex, _BufferedBoneCount);
		}

		//!!!this allows using _ConstSkinPalette curcularly with no_overwrite! if out of space, wrap and discard and start filling from beginning. Hide inside CShaderParamStorage?
		//!!!make sure that only a part of the buffer is updated and submitted to GPU!
		const auto InstanceBoneCount = std::min(Model.BoneCount, _ConstSkinPalette.GetElementCount() - _BufferedBoneCount);
		_PerInstance.SetMatrixArray(_ConstSkinPalette, Model.pSkinPalette, InstanceBoneCount, _BufferedBoneCount);
		_BufferedBoneCount += InstanceBoneCount;
	}

	if (_MemberLightCount)
	{
		const auto LightCount = LightingEnabled ? std::min<U32>(Model.LightCount, _MemberLightIndices.GetElementCount()) : 0;

		//???need or use INVALID_INDEX to stop iterating light index array in a shader? possibly uses less shader consts!
		_MemberLightCount.Shift(_ConstInstanceData, _InstanceCount);
		_PerInstance.SetUInt(_MemberLightCount, LightCount);

		//???send all or only visible lights to GPU? how to detect that something have changed, to avoid resending each frame?
		//set flag when testing lights against frustum and visibility of one actually changes?

		// Set per-instance light indices
		if (LightCount)
		{
			//CArray<U16>::CIterator ItIdx = Context.pLightIndices->IteratorAt(Model.LightIndexBase);
			//U32 InstLightIdx;
			//for (InstLightIdx = 0; InstLightIdx < Model.LightCount; ++InstLightIdx, ++ItIdx)
			//{
			//	const CLightRecord& LightRec = (*Context.pLights)[(*ItIdx)];
			//	PerInstance.SetInt(CurrLightIndices.GetComponent(InstLightIdx), LightRec.GPULightIndex);
			//}
		}
	}

	++_InstanceCount;

	//!!!updating big CB loads the bus with unnecessary bytes. Using big per-instance data array for few instances will pass too many unnecessary traffic.
	//what about UpdateSubresource? Can it make this better? Could exploit no-overwrite and offsets to avoid stalls drawing from previous region!
	//???also could use a pool of buffers of different sizes, and bind shorter buffer than shader expects, guaranteeing that extra data is not accessed?
	//Q: What is the best way to update constant buffers?
	//A: UpdateSubresource and Map with Discard should be about the same speed. Choose between them depending on which one copies the least amount of memory.
	//   If you already have your data stored in memory in one contiguous block, use UpdateSubresource.If you need to accumulate data from other places, use Map with Discard.
	// From McDonald: UpdateSubresource requires more CPU time. When possible, prefer Map / Unmap. WTF?

	//!!!skin palettes can be pre-uploaded in z prepass and reused in color pass! But may not work well with instancing.
	//Q: How much can I improve my frame rate if I only upload my character's bones once per frame instead of once per pass/draw?
	//A: You can improve frame rate between 8 percent and 50 percent depending on the amount of redundant data.In the worst case, performance will not be reduced.

	//???go further and pack instance world matrices into skin palette?! de facto it is just the same but without actual skinning (degenerate skinning w/1 bone).
	//But cbuffer is better than tbuffer when indexing the same element. Or not anymore? At least can use cbuffer in non-skinned shader and tbuffer
	//in skinned, but with the same name "world matrices" or like that, and thus simplify the logic here. Need to think it over.

	//???calc skinned normal with float3x3 or as vOutput.vNor += mul( float4(vInput.vNor, 0.0f), amPalette[ aiIndices[ iBone  ] ] ) * fWeight; ?
	//see Khronos shaders, Skinning10 example shaders etc.
	//!!!when skinning, skin position and normal in the same loop! Need only one fetch per bone matrix then, and less control instructions! or check optimization!
	//!!!use float4x3 for skinning! and for world matrix too? can pack instance world matrix as 4x3 and then unpack in a shader adding 0001? or simplify mul?
	//???tbuffer or StructuredBuffer for bones? the second can change its size in runtime?
}
//---------------------------------------------------------------------

void CModelRenderer::EndRange(const CRenderContext& Context)
{
	if (_InstanceCount) CommitCollectedInstances();

	_PerInstance = {}; // FIXME: remove when moved to tech cache!
}
//---------------------------------------------------------------------

void CModelRenderer::CommitCollectedInstances()
{
	//!!!PERF: needs testing on big scene. Check is moved outside the call to reduce redundant call count, but is it necessary?
	n_assert_dbg(_InstanceCount);

	_PerInstance.Apply();

	//???need multipass techs or move that to another layer of logic?! multipass tech kills shader sorting and leads to render state switches.
	//but it keeps material and mesh, and maybe even cached intermediate data like GPU skinned vertices buffer.
	//???what effects use multipass techs at all? is there any not implementable with different render phases?
	//may remove pass arrays from tech and leave there only one render state, or at least a set of states for different factor (instance limit etc)
	// FIXME: get rid of light count variations? Or use them?
	UPTR LightCount = 0;
	const auto& Passes = _pCurrTech->GetPasses(LightCount);
	for (const auto& Pass : Passes)
	{
		_pGPU->SetRenderState(Pass);
		if (_InstanceCount > 1)
			_pGPU->DrawInstanced(*_pCurrGroup, _InstanceCount);
		else
			_pGPU->Draw(*_pCurrGroup); //!!!TODO PERF: check if this is better for a single object! DrawInstanced(1) works either! Maybe there is no profit in branching here!
	}

	_InstanceCount = 0;
	_BufferedBoneCount = 0;
}
//---------------------------------------------------------------------

}
