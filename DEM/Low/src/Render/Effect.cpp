#include "Effect.h"
#include <Render/GPUDriver.h>
#include <Render/Sampler.h>
#include <Render/ShaderConstant.h>
#include <Render/Texture.h>
#include <Render/RenderState.h>
#include <Render/RenderStateDesc.h>
#include <Render/SamplerDesc.h>
#include <Render/Shader.h>
#include <Render/ShaderMetadata.h>
#include <IO/BinaryReader.h>
#include <Data/StringUtils.h>

namespace Render
{
CEffect::CEffect() {}

CEffect::~CEffect()
{
	SAFE_FREE(pMaterialConstDefaultValues);
}
//---------------------------------------------------------------------

// Out array will be sorted by ID as parameters are saved sorted by ID
bool CEffect::LoadParams(IO::CBinaryReader& Reader,
	Render::CGPUDriver& GPU,
	CFixedArray<Render::CEffectConstant>& OutConsts,
	CFixedArray<Render::CEffectResource>& OutResources,
	CFixedArray<Render::CEffectSampler>& OutSamplers)
{
	U32 ParamCount;

	if (!Reader.Read<U32>(ParamCount)) FAIL;
	OutConsts.SetSize(ParamCount);
	for (UPTR ParamIdx = 0; ParamIdx < ParamCount; ++ParamIdx)
	{
		CStrID ParamID;
		if (!Reader.Read(ParamID)) FAIL;

		U8 ShaderType;
		if (!Reader.Read(ShaderType)) FAIL;

		U32 SourceShaderID;
		if (!Reader.Read(SourceShaderID)) FAIL;

		U8 ConstType;
		if (!Reader.Read(ConstType)) FAIL;

		//???!!!need to save-load?!
		U32 SizeInBytes;
		if (!Reader.Read(SizeInBytes)) FAIL;

		const Render::IShaderMetadata* pShaderMeta;
		if (SourceShaderID)
		{
			//!!!DBG TMP!
			CStrID UID = CStrID("ShLib:#" + StringUtils::FromInt(SourceShaderID));

			Render::PShader ParamShader = GPU.GetShader(UID);
			pShaderMeta = ParamShader->GetMetadata();
			//!!!fill MetadataShaders.
		}
		if (!pShaderMeta) FAIL;

		Render::HConst hConst = pShaderMeta->GetConstHandle(ParamID);
		if (hConst == INVALID_HANDLE) FAIL;

		Render::CEffectConstant& Rec = OutConsts[ParamIdx];
		Rec.ID = ParamID;
		Rec.ShaderType = (Render::EShaderType)ShaderType;
		Rec.Const = pShaderMeta->GetConstant(hConst);
	}

	if (!Reader.Read<U32>(ParamCount)) FAIL;
	OutResources.SetSize(ParamCount);
	for (UPTR ParamIdx = 0; ParamIdx < ParamCount; ++ParamIdx)
	{
		CStrID ParamID;
		if (!Reader.Read(ParamID)) FAIL;

		U8 ShaderType;
		if (!Reader.Read(ShaderType)) FAIL;

		U32 SourceShaderID;
		if (!Reader.Read(SourceShaderID)) FAIL;

		const Render::IShaderMetadata* pShaderMeta;
		if (SourceShaderID)
		{
			//!!!DBG TMP!
			CStrID UID = CStrID("ShLib:#" + StringUtils::FromInt(SourceShaderID));

			// Shader will stay alive in a cache, so metadata will be valid
			Render::PShader ParamShader = GPU.GetShader(UID);
			pShaderMeta = ParamShader->GetMetadata();
			//!!!fill MetadataShaders.
		}
		if (!pShaderMeta) FAIL;

		Render::CEffectResource& Rec = OutResources[ParamIdx];
		Rec.ID = ParamID;
		Rec.Handle = pShaderMeta->GetResourceHandle(ParamID);
		Rec.ShaderType = (Render::EShaderType)ShaderType;
	}

	if (!Reader.Read<U32>(ParamCount)) FAIL;
	OutSamplers.SetSize(ParamCount);
	for (UPTR ParamIdx = 0; ParamIdx < ParamCount; ++ParamIdx)
	{
		CStrID ParamID;
		if (!Reader.Read(ParamID)) FAIL;

		U8 ShaderType;
		if (!Reader.Read(ShaderType)) FAIL;

		U32 SourceShaderID;
		if (!Reader.Read(SourceShaderID)) FAIL;

		const Render::IShaderMetadata* pShaderMeta;
		if (SourceShaderID)
		{
			//!!!DBG TMP!
			CStrID UID = CStrID("ShLib:#" + StringUtils::FromInt(SourceShaderID));

			// Shader will stay alive in a cache, so metadata will be valid
			Render::PShader ParamShader = GPU.GetShader(UID);
			pShaderMeta = ParamShader->GetMetadata();
			//!!!fill MetadataShaders.
		}
		if (!pShaderMeta) FAIL;

		Render::CEffectSampler& Rec = OutSamplers[ParamIdx];
		Rec.ID = ParamID;
		Rec.Handle = pShaderMeta->GetSamplerHandle(ParamID);
		Rec.ShaderType = (Render::EShaderType)ShaderType;
	}

	OK;
}
//---------------------------------------------------------------------

bool CEffect::LoadParamValues(IO::CBinaryReader& Reader,
	Render::CGPUDriver& GPU,
	CDict<CStrID, void*>& OutConsts,
	CDict<CStrID, Render::PTexture>& OutResources,
	CDict<CStrID, Render::PSampler>& OutSamplers,
	void*& pOutConstValueBuffer)
{
	U32 ParamCount;
	if (!Reader.Read<U32>(ParamCount)) FAIL;
	for (UPTR ParamIdx = 0; ParamIdx < ParamCount; ++ParamIdx)
	{
		CStrID ParamID;
		if (!Reader.Read(ParamID)) FAIL;

		U8 Type;
		if (!Reader.Read(Type)) FAIL;

		switch (Type)
		{
			case Render::EPT_Const:
			{
				U32 Offset;
				if (!Reader.Read(Offset)) FAIL;
				if (!OutConsts.IsInAddMode()) OutConsts.BeginAdd();
				OutConsts.Add(ParamID, (void*)Offset);
				break;
			}
			case Render::EPT_Resource:
			{
				CStrID RUID;
				if (!Reader.Read(RUID)) FAIL;
				Render::PTexture Texture = GPU.GetTexture(RUID, Render::Access_GPU_Read);
				if (Texture.IsNullPtr()) FAIL;
				if (!OutResources.IsInAddMode()) OutResources.BeginAdd();
				OutResources.Add(ParamID, Texture);
				break;
			}
			case Render::EPT_Sampler:
			{
				Render::CSamplerDesc SamplerDesc;

				U8 U8Value;
				Reader.Read<U8>(U8Value);
				SamplerDesc.AddressU = (Render::ETexAddressMode)U8Value;
				Reader.Read<U8>(U8Value);
				SamplerDesc.AddressV = (Render::ETexAddressMode)U8Value;
				Reader.Read<U8>(U8Value);
				SamplerDesc.AddressW = (Render::ETexAddressMode)U8Value;
				Reader.Read<U8>(U8Value);
				SamplerDesc.Filter = (Render::ETexFilter)U8Value;

				Reader.Read(SamplerDesc.BorderColorRGBA[0]);
				Reader.Read(SamplerDesc.BorderColorRGBA[1]);
				Reader.Read(SamplerDesc.BorderColorRGBA[2]);
				Reader.Read(SamplerDesc.BorderColorRGBA[3]);
				Reader.Read(SamplerDesc.MipMapLODBias);
				Reader.Read(SamplerDesc.FinestMipMapLOD);
				Reader.Read(SamplerDesc.CoarsestMipMapLOD);
				Reader.Read(SamplerDesc.MaxAnisotropy);

				Reader.Read<U8>(U8Value);
				SamplerDesc.CmpFunc = (Render::ECmpFunc)U8Value;

				Render::PSampler Sampler = GPU.CreateSampler(SamplerDesc);
				if (Sampler.IsNullPtr()) FAIL;
				if (!OutSamplers.IsInAddMode()) OutSamplers.BeginAdd();
				OutSamplers.Add(ParamID, Sampler);
				break;
			}
		}
	}

	if (OutConsts.IsInAddMode()) OutConsts.EndAdd();
	if (OutResources.IsInAddMode()) OutResources.EndAdd();
	if (OutSamplers.IsInAddMode()) OutSamplers.EndAdd();

	U32 ValueBufferSize;
	if (!Reader.Read(ValueBufferSize)) FAIL;
	if (ValueBufferSize)
	{
		pOutConstValueBuffer = n_malloc(ValueBufferSize);
		Reader.GetStream().Read(pOutConstValueBuffer, ValueBufferSize);
		//???return ValueBufferSize too?

		for (UPTR i = 0; i < OutConsts.GetCount(); ++i)
		{
			void*& pValue = OutConsts.ValueAt(i);
			pValue = (char*)pOutConstValueBuffer + (U32)pValue;
		}
	}
	else pOutConstValueBuffer = NULL;

	OK;
}
//---------------------------------------------------------------------

static bool SkipEffectParams(IO::CBinaryReader& Reader)
{
	// Constants
	U32 Count;
	if (!Reader.Read(Count)) FAIL;
	for (U32 Idx = 0; Idx < Count; ++Idx)
	{
		CString StrValue;
		if (!Reader.Read(StrValue)) FAIL;
		if (!Reader.GetStream().Seek(10, IO::Seek_Current)) FAIL;
	}

	// Resources
	if (!Reader.Read(Count)) FAIL;
	for (U32 Idx = 0; Idx < Count; ++Idx)
	{
		CString StrValue;
		if (!Reader.Read(StrValue)) FAIL;
		if (!Reader.GetStream().Seek(5, IO::Seek_Current)) FAIL;
	}

	// Samplers
	if (!Reader.Read(Count)) FAIL;
	for (U32 Idx = 0; Idx < Count; ++Idx)
	{
		CString StrValue;
		if (!Reader.Read(StrValue)) FAIL;
		if (!Reader.GetStream().Seek(5, IO::Seek_Current)) FAIL;
	}

	OK;
}
//---------------------------------------------------------------------

bool CEffect::Load(CGPUDriver& GPU, IO::CStream& Stream)
{
	if (IsValid()) FAIL; // For now can't reload, need to clear before. Users will become broken.

	IO::CBinaryReader Reader(Stream);

	U32 Magic;
	if (!Reader.Read<U32>(Magic) || Magic != 'SHFX') FAIL;

	U32 Version;
	if (!Reader.Read<U32>(Version)) FAIL;

	U32 ShaderModel;
	if (!Reader.Read<U32>(ShaderModel)) FAIL; // 0 for SM3.0, 1 for USM

	U32 EffectType;
	if (!Reader.Read<U32>(EffectType)) FAIL;

	CFixedArray<CFixedArray<Render::PRenderState>> RenderStates; // By render state index, by variation
	U32 RSCount;
	if (!Reader.Read<U32>(RSCount)) FAIL;
	RenderStates.SetSize(RSCount);
	for (UPTR i = 0; i < RSCount; ++i)
	{
		Render::CRenderStateDesc Desc;
		Desc.SetDefaults();

		U32 MaxLights;
		if (!Reader.Read(MaxLights)) FAIL;
		UPTR LightVariationCount = MaxLights + 1;

		U8 U8Value;
		U32 U32Value;

		if (!Reader.Read(U32Value)) FAIL;
		Desc.Flags.ResetTo(U32Value);

		if (!Reader.Read(Desc.DepthBias)) FAIL;
		if (!Reader.Read(Desc.DepthBiasClamp)) FAIL;
		if (!Reader.Read(Desc.SlopeScaledDepthBias)) FAIL;

		if (Desc.Flags.Is(Render::CRenderStateDesc::DS_DepthEnable))
		{
			if (!Reader.Read<U8>(U8Value)) FAIL;
			Desc.DepthFunc = (Render::ECmpFunc)U8Value;
		}

		if (Desc.Flags.Is(Render::CRenderStateDesc::DS_StencilEnable))
		{
			if (!Reader.Read(Desc.StencilReadMask)) FAIL;
			if (!Reader.Read(Desc.StencilWriteMask)) FAIL;
			if (!Reader.Read<U32>(U32Value)) FAIL;
			Desc.StencilRef = U32Value;

			if (!Reader.Read<U8>(U8Value)) FAIL;
			Desc.StencilFrontFace.StencilFailOp = (Render::EStencilOp)U8Value;
			if (!Reader.Read<U8>(U8Value)) FAIL;
			Desc.StencilFrontFace.StencilDepthFailOp = (Render::EStencilOp)U8Value;
			if (!Reader.Read<U8>(U8Value)) FAIL;
			Desc.StencilFrontFace.StencilPassOp = (Render::EStencilOp)U8Value;
			if (!Reader.Read<U8>(U8Value)) FAIL;
			Desc.StencilFrontFace.StencilFunc = (Render::ECmpFunc)U8Value;

			if (!Reader.Read<U8>(U8Value)) FAIL;
			Desc.StencilBackFace.StencilFailOp = (Render::EStencilOp)U8Value;
			if (!Reader.Read<U8>(U8Value)) FAIL;
			Desc.StencilBackFace.StencilDepthFailOp = (Render::EStencilOp)U8Value;
			if (!Reader.Read<U8>(U8Value)) FAIL;
			Desc.StencilBackFace.StencilPassOp = (Render::EStencilOp)U8Value;
			if (!Reader.Read<U8>(U8Value)) FAIL;
			Desc.StencilBackFace.StencilFunc = (Render::ECmpFunc)U8Value;
		}

		for (UPTR BlendIdx = 0; BlendIdx < 8; ++BlendIdx)
		{
			if (BlendIdx > 0 && Desc.Flags.IsNot(Render::CRenderStateDesc::Blend_Independent)) break;

			Render::CRenderStateDesc::CRTBlend& RTBlend = Desc.RTBlend[BlendIdx];

			if (!Reader.Read(RTBlend.WriteMask)) FAIL;

			if (Desc.Flags.IsNot(Render::CRenderStateDesc::Blend_RTBlendEnable << BlendIdx)) continue;

			if (!Reader.Read<U8>(U8Value)) FAIL;
			RTBlend.SrcBlendArg = (Render::EBlendArg)U8Value;
			if (!Reader.Read<U8>(U8Value)) FAIL;
			RTBlend.DestBlendArg = (Render::EBlendArg)U8Value;
			if (!Reader.Read<U8>(U8Value)) FAIL;
			RTBlend.BlendOp = (Render::EBlendOp)U8Value;
			if (!Reader.Read<U8>(U8Value)) FAIL;
			RTBlend.SrcBlendArgAlpha = (Render::EBlendArg)U8Value;
			if (!Reader.Read<U8>(U8Value)) FAIL;
			RTBlend.DestBlendArgAlpha = (Render::EBlendArg)U8Value;
			if (!Reader.Read<U8>(U8Value)) FAIL;
			RTBlend.BlendOpAlpha = (Render::EBlendOp)U8Value;
		}

		if (!Reader.Read(Desc.BlendFactorRGBA[0])) FAIL;
		if (!Reader.Read(Desc.BlendFactorRGBA[1])) FAIL;
		if (!Reader.Read(Desc.BlendFactorRGBA[2])) FAIL;
		if (!Reader.Read(Desc.BlendFactorRGBA[3])) FAIL;
		if (!Reader.Read<U32>(U32Value)) FAIL;
		Desc.SampleMask = U32Value;

		if (!Reader.Read(Desc.AlphaTestRef)) FAIL;
		if (!Reader.Read<U8>(U8Value)) FAIL;
		Desc.AlphaTestFunc = (Render::ECmpFunc)U8Value;

		CFixedArray<Render::PRenderState>& Variations = RenderStates[i];
		Variations.SetSize(LightVariationCount);

		UPTR VariationArraySize = 0;
		for (UPTR LightCount = 0; LightCount < LightVariationCount; ++LightCount)
		{
			bool ShaderLoadingFailed = false;

			Render::PShader* pShaders[] = { &Desc.VertexShader, &Desc.PixelShader, &Desc.GeometryShader, &Desc.HullShader, &Desc.DomainShader };
			for (UPTR ShaderType = Render::ShaderType_Vertex; ShaderType < Render::ShaderType_COUNT; ++ShaderType)
			{
				U32 ShaderID;
				if (!Reader.Read<U32>(ShaderID)) FAIL;

				if (!ShaderID)
				{
					*pShaders[ShaderType] = NULL;
					continue;
				}

				NOT_IMPLEMENTED;
				//*pShaders[ShaderType] = ShaderLibrary->GetShaderByID(ShaderID);
			}

			Variations[LightCount] = ShaderLoadingFailed ? nullptr : GPU.CreateRenderState(Desc);
			if (Variations[LightCount].IsValidPtr()) VariationArraySize = LightCount + 1;
		}

		if (VariationArraySize < LightVariationCount)
			Variations.SetSize(VariationArraySize, true);
	}

	// Load techniques

	CDict<CStrID, Render::PTechnique> Techs;

	Render::EGPUFeatureLevel GPULevel = GPU.GetFeatureLevel();

	U32 TechCount;
	if (!Reader.Read<U32>(TechCount) || !TechCount) FAIL;

	CStrID CurrInputSet;
	UPTR ShaderInputSetID;
	bool BestTechFound = false; // True if we already loaded the best tech for the current input set

	for (UPTR i = 0; i < TechCount; ++i)
	{
		CStrID TechID;
		CStrID InputSet;
		U32 Target;
		if (!Reader.Read(TechID)) FAIL;
		if (!Reader.Read(InputSet)) FAIL;
		if (!Reader.Read(Target)) FAIL;

		U32 U32Value;
		if (!Reader.Read(U32Value)) FAIL;
		Render::EGPUFeatureLevel MinFeatureLevel = (Render::EGPUFeatureLevel)U32Value;

		U64 RequiresFlags;
		if (!Reader.Read(RequiresFlags)) FAIL;

		if (InputSet != CurrInputSet)
		{
			CurrInputSet = InputSet;
			ShaderInputSetID = Render::RegisterShaderInputSetID(InputSet);
			BestTechFound = false;
		}

		// Check for hardware and API support. For low-performance graphic cards with rich feature
		// support, a better way is to measure GPU performance on driver init and choose here not
		// only by features supported, but by performance 'score' of tech vs GPU too.
		//!!!check RequiresFlags!
		if (BestTechFound || MinFeatureLevel > GPULevel || !GPU.SupportsShaderModel(Target))
		{
			U32 PassCount;
			if (!Reader.Read<U32>(PassCount)) FAIL;
			if (!Stream.Seek(4 * PassCount, IO::Seek_Current)) FAIL; // Pass indices
			U32 MaxLights;
			if (!Reader.Read<U32>(MaxLights)) FAIL;
			if (!Stream.Seek(MaxLights + 1, IO::Seek_Current)) FAIL; // VariationValid flags
			if (!SkipEffectParams(Reader)) FAIL;
			continue;
		}

		Render::PTechnique Tech = n_new(Render::CTechnique);
		Tech->Name = TechID;
		Tech->ShaderInputSetID = ShaderInputSetID;
		//Tech->MinFeatureLevel = MinFeatureLevel;

		bool HasCompletelyInvalidPass = false;

		U32 PassCount;
		if (!Reader.Read<U32>(PassCount)) FAIL;
		CFixedArray<U32> PassRenderStateIndices(PassCount);
		for (UPTR PassIdx = 0; PassIdx < PassCount; ++PassIdx)
		{
			U32 PassRenderStateIdx;
			if (!Reader.Read<U32>(PassRenderStateIdx)) FAIL;

			//!!!can lazy-load render states here! read desc above, but don't create GPU objects!
			//many unused render states may be loaded unnecessarily

			if (PassRenderStateIdx == INVALID_INDEX || !RenderStates[PassRenderStateIdx].GetCount())
			{
				HasCompletelyInvalidPass = true;
				break;
			}

			PassRenderStateIndices[PassIdx] = PassRenderStateIdx;
		}

		U32 MaxLights;
		if (!Reader.Read<U32>(MaxLights)) FAIL;

		if (HasCompletelyInvalidPass)
		{
			if (!Stream.Seek(MaxLights + 1, IO::Seek_Current)) FAIL; // VariationValid flags
			if (!SkipEffectParams(Reader)) FAIL;
			continue;
		}

		UPTR NewVariationCount = 0;
		UPTR ValidVariationCount = 0;
		UPTR VariationCount = MaxLights + 1;

		Tech->PassesByLightCount.SetSize(VariationCount);

		for (UPTR LightCount = 0; LightCount < VariationCount; ++LightCount)
		{
			// Always 0 or 1
			U8 VariationValid;
			if (!Reader.Read<U8>(VariationValid)) FAIL;

			Render::CPassList& PassList = Tech->PassesByLightCount[LightCount];
			PassList.SetSize(PassCount);

			if (VariationValid)
			{
				for (UPTR PassIdx = 0; PassIdx < PassCount; ++PassIdx)
				{
					Render::PRenderState RS = RenderStates[PassRenderStateIndices[PassIdx]][LightCount];
					if (RS.IsNullPtr())
					{
						// Valid variation references failed render state
						VariationValid = false;
						break;
					}
					PassList[PassIdx] = RS;
				}
			}

			if (VariationValid)
			{
				NewVariationCount = LightCount + 1;
				++ValidVariationCount;
			}
			else
			{
				PassList.Clear();
				// if LightCount is more than pass allows, truncate MaxLights to LightCount - 1
				// if LightCount was 0 in this case, fail tech
			}

		}

		if (!NewVariationCount)
		{
			// No valid variations in a tech
			if (!SkipEffectParams(Reader)) FAIL;
			continue;
		}
		else if (NewVariationCount < VariationCount)
			Tech->PassesByLightCount.SetSize(NewVariationCount, true);

		// If all tech variations are valid, it is considered the best, because they
		// are sorted by richness of features and all subsequent techs are not as good
		if (VariationCount == ValidVariationCount) BestTechFound = true;

		IPTR TechDictIdx = Techs.FindIndex(InputSet);
		if (TechDictIdx == INVALID_INDEX) Techs.Add(InputSet, Tech);
		else
		{
			// Compare with already loaded tech, isn't it better. We intentionally keep only one
			// tech per InputSet loaded. Someone may want to load more techs for a particular
			// InputSet and choose by name, but it is considered a rare case for a game.
			// Tech is considered better if it has more valid variations.
			bool ReplaceExistingTech = BestTechFound;
			if (!ReplaceExistingTech)
			{
				Render::PTechnique ExistingTech = Techs.ValueAt(TechDictIdx);
				UPTR ExistingValidVariationCount = 0;
				for (UPTR ExVarIdx = 0; ExVarIdx < ExistingTech->PassesByLightCount.GetCount(); ++ExVarIdx)
					if (ExistingTech->PassesByLightCount[ExVarIdx].GetCount())
						++ExistingValidVariationCount;
				ReplaceExistingTech = (ValidVariationCount > ExistingValidVariationCount);
			}

			if (ReplaceExistingTech) Techs.ValueAt(TechDictIdx) = Tech;
			else
			{
				if (!SkipEffectParams(Reader)) FAIL;
				continue;
			}
		}

		// Load tech params info

		if (!LoadParams(Reader, GPU, Tech->Consts, Tech->Resources, Tech->Samplers)) FAIL;
	}

	if (!Techs.GetCount()) FAIL;

	//!!!try to find by effect ID! add into it, if found!
	//so GPU will find loaded effect & add loaded data into it. Is still actual?
	switch (EffectType)
	{
		case 0:		Type = Render::EffectType_Opaque; break;
		case 1:		Type = Render::EffectType_AlphaTest; break;
		case 2:		Type = Render::EffectType_Skybox; break;
		case 3:		Type = Render::EffectType_AlphaBlend; break;
		default:	Type = Render::EffectType_Other; break;
	}

	// Skip global params

	if (!SkipEffectParams(Reader)) FAIL;

	// Load material params

	if (!LoadParams(Reader, GPU, MaterialConsts, MaterialResources, MaterialSamplers)) FAIL;

	//???save precalc in a tool?
	CArray<HHandle> MtlConstBuffers;
	for (UPTR i = 0; i < MaterialConsts.GetCount(); ++i)
	{
		const Render::HConstBuffer hCB = MaterialConsts[i].Const->GetConstantBufferHandle();
		if (!MtlConstBuffers.Contains(hCB)) MtlConstBuffers.Add(hCB);
	}
	MaterialConstantBufferCount = MtlConstBuffers.GetCount();
	MtlConstBuffers.Clear(true);

	void* pVoidBuffer;
	if (!LoadParamValues(Reader, GPU, DefaultConsts, DefaultResources, DefaultSamplers, pVoidBuffer)) FAIL;
	pMaterialConstDefaultValues = (char*)pVoidBuffer;

	// Build tech indices
	TechsByName.BeginAdd(Techs.GetCount());
	TechsByInputSet.BeginAdd(Techs.GetCount());
	for (UPTR i = 0; i < Techs.GetCount(); ++i)
	{
		auto& Tech = Techs.ValueAt(i);
		TechsByName.Add(Tech->GetName(), Tech);
		TechsByInputSet.Add(Tech->GetShaderInputSetID(), Tech);
	}
	TechsByName.EndAdd();
	TechsByInputSet.EndAdd();

	OK;
}
//---------------------------------------------------------------------

const CTechnique* CEffect::GetTechByName(CStrID Name) const
{
	IPTR Idx = TechsByName.FindIndex(Name);
	return Idx == INVALID_INDEX ? nullptr : TechsByName.ValueAt(Idx).Get();
}
//---------------------------------------------------------------------

const CTechnique* CEffect::GetTechByInputSet(UPTR InputSet) const
{
	IPTR Idx = TechsByInputSet.FindIndex(InputSet);
	return Idx == INVALID_INDEX ? nullptr : TechsByInputSet.ValueAt(Idx).Get();
}
//---------------------------------------------------------------------

void* CEffect::GetConstantDefaultValue(CStrID ID) const
{
	IPTR Idx = DefaultConsts.FindIndex(ID);
	return Idx == INVALID_INDEX ? nullptr : DefaultConsts.ValueAt(Idx);
}
//---------------------------------------------------------------------

PTexture CEffect::GetResourceDefaultValue(CStrID ID) const
{
	IPTR Idx = DefaultResources.FindIndex(ID);
	return Idx == INVALID_INDEX ? NULL : DefaultResources.ValueAt(Idx);
}
//---------------------------------------------------------------------

PSampler CEffect::GetSamplerDefaultValue(CStrID ID) const
{
	IPTR Idx = DefaultSamplers.FindIndex(ID);
	return Idx == INVALID_INDEX ? NULL : DefaultSamplers.ValueAt(Idx);
}
//---------------------------------------------------------------------

}