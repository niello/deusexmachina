#include "ModelRenderer.h"
#include <Render/Model.h>
#include <Render/Material.h>
#include <Render/Effect.h>
#include <Render/Mesh.h>
#include <Render/Light.h>
#include <Render/GPUDriver.h>
#include <Core/Factory.h>

namespace Render
{
FACTORY_CLASS_IMPL(Render::CModelRenderer, 'MDLR', Render::IRenderer);

CModelRenderer::CModelTechInterface* CModelRenderer::GetTechInterface(const CTechnique* pTech)
{
	auto It = _TechInterfaces.find(pTech);
	if (It != _TechInterfaces.cend()) return &It->second;

	auto& TechInterface = _TechInterfaces[pTech];
	TechInterface.TechLightCount = 0;

	if (pTech->GetParamTable().HasParams())
	{
		static const CStrID sidInstanceData("InstanceData");
		static const CStrID sidSkinPalette("SkinPalette");
		static const CStrID sidWorldMatrix("WorldMatrix");
		static const CStrID sidFirstBoneIndex("FirstBoneIndex");
		static const CStrID sidLightIndices("LightIndices");
		static const CStrID sidLightCount("LightCount");

		TechInterface.PerInstanceParams = CShaderParamStorage(pTech->GetParamTable(), *_pGPU);

		TechInterface.ConstInstanceData = pTech->GetParamTable().GetConstant(sidInstanceData);
		TechInterface.ConstSkinPalette = pTech->GetParamTable().GetConstant(sidSkinPalette);

		if (auto Struct = TechInterface.ConstInstanceData[0])
		{
			TechInterface.MemberFirstBoneIndex = Struct[sidFirstBoneIndex];
			TechInterface.MemberWorldMatrix = Struct[sidWorldMatrix];
			TechInterface.MemberLightIndices = Struct[sidLightIndices];

			// Extend the common buffer if necessary
			if (TechInterface.MemberLightIndices)
			{
				TechInterface.TechLightCount = TechInterface.MemberLightIndices.GetTotalComponentCount();
				_LightIndexBuffer.reserve(TechInterface.TechLightCount);
			}
		}
	}

	TechInterface.TechMaxInstanceCount = TechInterface.ConstInstanceData ? std::max<U32>(1, TechInterface.ConstInstanceData.GetElementCount()) : std::numeric_limits<U32>::max();
	TechInterface.TechNeedsMaterial = pTech->GetEffect()->GetMaterialParamTable().HasParams();

	return &TechInterface;
}
//---------------------------------------------------------------------

bool CModelRenderer::BeginRange(const CRenderContext& Context)
{
	_pCurrTech = nullptr;
	_pCurrTechInterface = nullptr;
	_pCurrMaterial = nullptr;
	_pCurrMesh = nullptr;
	_pCurrGroup = nullptr;
	_pGPU = Context.pGPU; // FIXME: could instead pass CRenderContext to accessing methods
	_InstanceCount = 0;

	OK;
}
//---------------------------------------------------------------------

// For constant buffer handling see https://learn.microsoft.com/en-us/windows/win32/dxtecharts/direct3d10-frequently-asked-questions
void CModelRenderer::Render(const CRenderContext& Context, IRenderable& Renderable)
{
	ZoneScoped;

	CModel& Model = static_cast<CModel&>(Renderable);

	const CTechnique* pTech = Context.pShaderTechCache[Model.ShaderTechIndex];
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
		_pCurrTechInterface = GetTechInterface(pTech);
	}
	else
	{
		// Check if the limit of per-instance data for the tech is reached. Commit if either:
		// - constant buffer with instance data structures is filled
		// - the model is skinned and the tech doesn't support skinned instancing
		// - model's bones do not fit into the remaining part of the shader skin palette buffer which is already not empty
		if (_InstanceCount == _pCurrTechInterface->TechMaxInstanceCount ||
			(_InstanceCount && Model.BoneCount && !_pCurrTechInterface->MemberFirstBoneIndex) ||
			(_BufferedBoneCount && Model.BoneCount > _pCurrTechInterface->ConstSkinPalette.GetElementCount() - _BufferedBoneCount))
		{
			CommitCollectedInstances();
		}
	}

	if (_pCurrTechInterface->TechNeedsMaterial && pMaterial != _pCurrMaterial)
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

	if (_pCurrTechInterface->MemberWorldMatrix)
	{
		_pCurrTechInterface->MemberWorldMatrix.Shift(_pCurrTechInterface->ConstInstanceData, _InstanceCount);
		_pCurrTechInterface->PerInstanceParams.SetMatrix(_pCurrTechInterface->MemberWorldMatrix, Model.Transform);
	}

	if (Model.pSkinPalette)
	{
		if (_pCurrTechInterface->MemberFirstBoneIndex)
		{
			_pCurrTechInterface->MemberFirstBoneIndex.Shift(_pCurrTechInterface->ConstInstanceData, _InstanceCount);
			_pCurrTechInterface->PerInstanceParams.SetUInt(_pCurrTechInterface->MemberFirstBoneIndex, _BufferedBoneCount);
		}

		//!!!this allows using _ConstSkinPalette curcularly with no_overwrite! if out of space, wrap and discard and start filling from beginning. Hide inside CShaderParamStorage?
		//!!!make sure that only a changed part of the buffer is updated and submitted to GPU!
		const auto InstanceBoneCount = std::min(Model.BoneCount, _pCurrTechInterface->ConstSkinPalette.GetElementCount() - _BufferedBoneCount);
		_pCurrTechInterface->PerInstanceParams.SetMatrixArray(_pCurrTechInterface->ConstSkinPalette, Model.pSkinPalette, InstanceBoneCount, _BufferedBoneCount);
		_BufferedBoneCount += InstanceBoneCount;
	}

	if (_pCurrTechInterface->TechLightCount)
	{
		// Set per-instance light indices for currently visible lights, clamp by limit
		_LightIndexBuffer.clear();
		_pCurrTechInterface->MemberLightIndices.Shift(_pCurrTechInterface->ConstInstanceData, _InstanceCount);
		for (const auto& Pair : Model.Lights)
		{
			const auto GPUIndex = Pair.second->GPUIndex;
			if (GPUIndex != INVALID_INDEX_T<U32>)
			{
				_LightIndexBuffer.push_back(GPUIndex);
				if (_LightIndexBuffer.size() == _pCurrTechInterface->TechLightCount) break;
			}
		}

		if (_LightIndexBuffer.size() < _pCurrTechInterface->TechLightCount)
			_LightIndexBuffer.push_back(-1);

		_pCurrTechInterface->PerInstanceParams.SetRawConstant(_pCurrTechInterface->MemberLightIndices, _LightIndexBuffer.data(), sizeof(U32) * _LightIndexBuffer.size());
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
}
//---------------------------------------------------------------------

void CModelRenderer::CommitCollectedInstances()
{
	//!!!PERF: needs testing on big scene. Check is moved outside the call to reduce redundant call count, but is it necessary?
	n_assert_dbg(_InstanceCount);

	_pCurrTechInterface->PerInstanceParams.Apply();

	//???need multipass techs or move that to another layer of logic?! multipass tech kills shader sorting and leads to render state switches.
	//but it keeps material and mesh, and maybe even cached intermediate data like GPU skinned vertices buffer.
	//???what effects use multipass techs at all? is there any not implementable with different render phases?
	//may remove pass arrays from tech and leave there only one render state, or at least a set of states for different factor (instance limit etc)
	for (const auto& Pass : _pCurrTech->GetPasses())
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
