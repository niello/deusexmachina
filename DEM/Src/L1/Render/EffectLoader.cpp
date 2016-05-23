#include "EffectLoader.h"

#include <Render/GPUDriver.h>
#include <Render/Effect.h>
#include <Render/RenderStateDesc.h>
#include <Render/SamplerDesc.h>
#include <Render/Texture.h>
#include <Render/TextureLoader.h>
#include <Render/Shader.h>
#include <Render/ShaderLoader.h>
#include <Render/ShaderLibrary.h>
#include <Resources/Resource.h>
#include <Resources/ResourceManager.h>
#include <IO/BinaryReader.h>
#include <IO/PathUtils.h>
#include <Data/StringUtils.h>

namespace Resources
{

PResourceLoader CEffectLoader::Clone()
{
	PEffectLoader NewLoader = n_new(CEffectLoader);
	NewLoader->GPU = GPU;
	NewLoader->ShaderLibrary = ShaderLibrary;
	return NewLoader.GetUnsafe();
}
//---------------------------------------------------------------------

const Core::CRTTI& CEffectLoader::GetResultType() const
{
	return Render::CEffect::RTTI;
}
//---------------------------------------------------------------------

//!!!can first load techs, then RS, not to load RS that won't be used!
PResourceObject CEffectLoader::Load(IO::CStream& Stream)
{
	if (GPU.IsNullPtr()) return NULL;

	IO::CBinaryReader Reader(Stream);

	U32 Magic;
	if (!Reader.Read<U32>(Magic) || Magic != 'SHFX') return NULL;

	U32 Version;
	if (!Reader.Read<U32>(Version)) return NULL;

	CFixedArray<CFixedArray<Render::PRenderState>> RenderStates; // By render state index, by variation
	U32 RSCount;
	if (!Reader.Read<U32>(RSCount)) return NULL;
	RenderStates.SetSize(RSCount);
	for (UPTR i = 0; i < RSCount; ++i)
	{
		Render::CRenderStateDesc Desc;
		Desc.SetDefaults();

		U32 MaxLights;
		if (!Reader.Read(MaxLights)) return NULL;
		UPTR LightVariationCount = MaxLights + 1;

		U8 U8Value;
		U32 U32Value;

		if (!Reader.Read(U32Value)) return NULL;
		Desc.Flags.ResetTo(U32Value);
		
		if (!Reader.Read(Desc.DepthBias)) return NULL;
		if (!Reader.Read(Desc.DepthBiasClamp)) return NULL;
		if (!Reader.Read(Desc.SlopeScaledDepthBias)) return NULL;
		
		if (Desc.Flags.Is(Render::CRenderStateDesc::DS_DepthEnable))
		{
			if (!Reader.Read<U8>(U8Value)) return NULL;
			Desc.DepthFunc = (Render::ECmpFunc)U8Value;
		}

		if (Desc.Flags.Is(Render::CRenderStateDesc::DS_StencilEnable))
		{
			if (!Reader.Read(Desc.StencilReadMask)) return NULL;
			if (!Reader.Read(Desc.StencilWriteMask)) return NULL;
			if (!Reader.Read<U32>(U32Value)) return NULL;
			Desc.StencilRef = U32Value;

			if (!Reader.Read<U8>(U8Value)) return NULL;
			Desc.StencilFrontFace.StencilFailOp = (Render::EStencilOp)U8Value;
			if (!Reader.Read<U8>(U8Value)) return NULL;
			Desc.StencilFrontFace.StencilDepthFailOp = (Render::EStencilOp)U8Value;
			if (!Reader.Read<U8>(U8Value)) return NULL;
			Desc.StencilFrontFace.StencilPassOp = (Render::EStencilOp)U8Value;
			if (!Reader.Read<U8>(U8Value)) return NULL;
			Desc.StencilFrontFace.StencilFunc = (Render::ECmpFunc)U8Value;

			if (!Reader.Read<U8>(U8Value)) return NULL;
			Desc.StencilBackFace.StencilFailOp = (Render::EStencilOp)U8Value;
			if (!Reader.Read<U8>(U8Value)) return NULL;
			Desc.StencilBackFace.StencilDepthFailOp = (Render::EStencilOp)U8Value;
			if (!Reader.Read<U8>(U8Value)) return NULL;
			Desc.StencilBackFace.StencilPassOp = (Render::EStencilOp)U8Value;
			if (!Reader.Read<U8>(U8Value)) return NULL;
			Desc.StencilBackFace.StencilFunc = (Render::ECmpFunc)U8Value;
		}

		for (UPTR BlendIdx = 0; BlendIdx < 8; ++BlendIdx)
		{
			if (BlendIdx > 0 && Desc.Flags.IsNot(Render::CRenderStateDesc::Blend_Independent)) break;
			if (Desc.Flags.IsNot(Render::CRenderStateDesc::Blend_RTBlendEnable << BlendIdx)) continue;

			Render::CRenderStateDesc::CRTBlend& RTBlend = Desc.RTBlend[BlendIdx];

			if (!Reader.Read<U8>(U8Value)) return NULL;
			RTBlend.SrcBlendArg = (Render::EBlendArg)U8Value;
			if (!Reader.Read<U8>(U8Value)) return NULL;
			RTBlend.DestBlendArg = (Render::EBlendArg)U8Value;
			if (!Reader.Read<U8>(U8Value)) return NULL;
			RTBlend.BlendOp = (Render::EBlendOp)U8Value;
			if (!Reader.Read<U8>(U8Value)) return NULL;
			RTBlend.SrcBlendArgAlpha = (Render::EBlendArg)U8Value;
			if (!Reader.Read<U8>(U8Value)) return NULL;
			RTBlend.DestBlendArgAlpha = (Render::EBlendArg)U8Value;
			if (!Reader.Read<U8>(U8Value)) return NULL;
			RTBlend.BlendOpAlpha = (Render::EBlendOp)U8Value;

			if (!Reader.Read(RTBlend.WriteMask)) return NULL;
		}

		if (!Reader.Read(Desc.BlendFactorRGBA[0])) return NULL;
		if (!Reader.Read(Desc.BlendFactorRGBA[1])) return NULL;
		if (!Reader.Read(Desc.BlendFactorRGBA[2])) return NULL;
		if (!Reader.Read(Desc.BlendFactorRGBA[3])) return NULL;
		if (!Reader.Read<U32>(U32Value)) return NULL;
		Desc.SampleMask = U32Value;

		if (!Reader.Read(Desc.AlphaTestRef)) return NULL;
		if (!Reader.Read<U8>(U8Value)) return NULL;
		Desc.AlphaTestFunc = (Render::ECmpFunc)U8Value;

		CFixedArray<Render::PRenderState>& Variations = RenderStates[i];
		Variations.SetSize(LightVariationCount);

		UPTR VariationArraySize = 0;
		for (UPTR LightCount = 0; LightCount < LightVariationCount; ++LightCount)
		{
			bool ShaderLoadingFailed = false;

			//???use .shd / .csh for all?
			const char* pExtension[] = { ".vsh", ".psh", ".gsh", ".hsh", ".dsh" };
			Render::PShader* pShaders[] = { &Desc.VertexShader, &Desc.PixelShader, &Desc.GeometryShader, &Desc.HullShader, &Desc.DomainShader };
			for (UPTR ShaderType = Render::ShaderType_Vertex; ShaderType < Render::ShaderType_COUNT; ++ShaderType)
			{
				U32 ShaderID;
				if (!Reader.Read<U32>(ShaderID)) return NULL;

				if (!ShaderID)
				{
					*pShaders[ShaderType] = NULL;
					continue;
				}

				//???try this way if no library or not loaded from library?
				//CString URI = "Shaders:Bin/" + StringUtils::FromInt(ShaderID) + pExtension[ShaderType];
				//Resources::PResource RShader = ResourceMgr->RegisterResource(URI.CStr());
				//if (!RShader->IsLoaded())
				//{
				//	Resources::PResourceLoader Loader = RShader->GetLoader();
				//	if (Loader.IsNullPtr())
				//		Loader = ResourceMgr->CreateDefaultLoaderFor<Render::CShader>(pExtension[ShaderType] + 1); // +1 to skip dot
				//	Loader->As<Resources::CShaderLoader>()->GPU = GPU;
				//	ResourceMgr->LoadResourceSync(*RShader, *Loader);
				//	if (!RShader->IsLoaded())
				//	{
				//		ShaderLoadingFailed = true;
				//		break;
				//	}
				//}

				//*pShaders[ShaderType] = RShader->GetObject<Render::CShader>();
				*pShaders[ShaderType] = ShaderLibrary->GetShaderByID(ShaderID);
			}

			Variations[LightCount] = ShaderLoadingFailed ? NULL : GPU->CreateRenderState(Desc);
			if (Variations[LightCount].IsValidPtr()) VariationArraySize = LightCount + 1;
		}

		if (VariationArraySize < LightVariationCount)
			Variations.SetSize(VariationArraySize, true);
	}

	// Load techniques

	CDict<CStrID, Render::PTechnique> Techs;

	Render::EGPUFeatureLevel GPULevel = GPU->GetFeatureLevel();

	U32 TechCount;
	if (!Reader.Read<U32>(TechCount) || !TechCount) return NULL;
	
	CStrID CurrInputSet;
	UPTR ShaderInputSetID;
	bool BestTechFound = false; // True if we already loaded the best tech for the current input set

	for (UPTR i = 0; i < TechCount; ++i)
	{
		CStrID TechID;
		CStrID InputSet;
		U32 Target;
		if (!Reader.Read(TechID)) return NULL;
		if (!Reader.Read(InputSet)) return NULL;
		if (!Reader.Read(Target)) return NULL;
		
		U32 U32Value;
		if (!Reader.Read(U32Value)) return NULL;
		Render::EGPUFeatureLevel MinFeatureLevel = (Render::EGPUFeatureLevel)U32Value;

		U64 RequiresFlags;
		if (!Reader.Read(RequiresFlags)) return NULL;

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
		if (BestTechFound || MinFeatureLevel > GPULevel || !GPU->SupportsShaderModel(Target))
		{
			U32 PassCount;
			if (!Reader.Read<U32>(PassCount)) return NULL;
			if (!Stream.Seek(4 * PassCount, IO::Seek_Current)) return NULL; // Pass indices
			U32 MaxLights;
			if (!Reader.Read<U32>(MaxLights)) return NULL;
			if (!Stream.Seek(MaxLights + 1, IO::Seek_Current)) return NULL; // VariationValid flags
			if (!SkipEffectParams(Reader)) return NULL;
			continue;
		}

		Render::PTechnique Tech = n_new(Render::CTechnique);
		Tech->Name = TechID;
		Tech->ShaderInputSetID = ShaderInputSetID;
		//Tech->MinFeatureLevel = MinFeatureLevel;

		bool HasCompletelyInvalidPass = false;

		U32 PassCount;
		if (!Reader.Read<U32>(PassCount)) return NULL;
		CFixedArray<U32> PassRenderStateIndices(PassCount);
		for (UPTR PassIdx = 0; PassIdx < PassCount; ++PassIdx)
		{
			U32 PassRenderStateIdx;
			if (!Reader.Read<U32>(PassRenderStateIdx)) return NULL;

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
		if (!Reader.Read<U32>(MaxLights)) return NULL;

		if (HasCompletelyInvalidPass)
		{
			if (!Stream.Seek(MaxLights + 1, IO::Seek_Current)) return NULL; // VariationValid flags
			if (!SkipEffectParams(Reader)) return NULL;
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
			if (!Reader.Read<U8>(VariationValid)) return NULL;

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
			if (!SkipEffectParams(Reader)) return NULL;
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
				if (!SkipEffectParams(Reader)) return NULL;
				continue;
			}
		}

		// Load tech params info

		CArray<CLoadedParam> Params;
		if (!LoadEffectParams(Reader, ShaderLibrary, Params)) return NULL;

		UPTR ConstCount = 0;
		UPTR ResourceCount = 0;
		UPTR SamplerCount = 0;
		for (UPTR i = 0; i < Params.GetCount(); ++i)
		{
			U8 Type = Params[i].Type;
			if (Type == Render::EPT_Const) ++ConstCount;
			else if (Type == Render::EPT_Resource) ++ResourceCount;
			else if (Type == Render::EPT_Sampler) ++SamplerCount;
		}
		Tech->Consts.SetSize(ConstCount);
		Tech->Resources.SetSize(ResourceCount);
		Tech->Samplers.SetSize(SamplerCount);

		ConstCount = 0;
		ResourceCount = 0;
		SamplerCount = 0;
		for (UPTR i = 0; i < Params.GetCount(); ++i)
		{
			CLoadedParam& Prm = Params[i];
			U8 Type = Prm.Type;
			if (Type == Render::EPT_Const)
			{
				Render::CTechConstant& Rec = Tech->Consts[ConstCount];
				Rec.ID = Prm.ID;
				Rec.Handle = Prm.Handle;
				Rec.BufferHandle = Prm.BufferHandle;
				Rec.ShaderType = (Render::EShaderType)Prm.ShaderType;
				++ConstCount;
			}
			else if (Type == Render::EPT_Resource)
			{
				Render::CTechResource& Rec = Tech->Resources[ResourceCount];
				Rec.ID = Prm.ID;
				Rec.Handle = Prm.Handle;
				Rec.ShaderType = (Render::EShaderType)Prm.ShaderType;
				++ResourceCount;
			}
			else if (Type == Render::EPT_Sampler)
			{
				Render::CTechSampler& Rec = Tech->Samplers[SamplerCount];
				Rec.ID = Prm.ID;
				Rec.Handle = Prm.Handle;
				Rec.ShaderType = (Render::EShaderType)Prm.ShaderType;
				++SamplerCount;
			}
		}
	}

	if (!Techs.GetCount()) return NULL;

	//!!!try to find by effect ID! add into it, if found!
	//!!!now rsrc mgmt doesn't support miltiple resources per file and partial resource definitions (multiple files per resource)!
	//may modify CEffectLoader to store all files of the effect, and use effect ID and rsrc URI separately.
	Render::PEffect Effect = n_new(Render::CEffect);

	// Load global and material params tables

	CArray<CLoadedParam> GlobalParams;
	if (!LoadEffectParams(Reader, ShaderLibrary, GlobalParams)) return NULL;

	//???param table or signature? need only for compatibility check when use effect with a render path!

	CArray<CLoadedParam> MtlParams;
	if (!LoadEffectParams(Reader, ShaderLibrary, MtlParams)) return NULL;

	UPTR ConstCount = 0;
	UPTR ResourceCount = 0;
	UPTR SamplerCount = 0;
	for (UPTR i = 0; i < MtlParams.GetCount(); ++i)
	{
		U8 Type = MtlParams[i].Type;
		if (Type == Render::EPT_Const) ++ConstCount;
		else if (Type == Render::EPT_Resource) ++ResourceCount;
		else if (Type == Render::EPT_Sampler) ++SamplerCount;
	}
	Effect->MaterialConsts.SetSize(ConstCount);
	Effect->MaterialResources.SetSize(ResourceCount);
	Effect->MaterialSamplers.SetSize(SamplerCount);

	CArray<Render::HConstBuffer> MtlConstBuffers;
	ConstCount = 0;
	ResourceCount = 0;
	SamplerCount = 0;
	for (UPTR i = 0; i < MtlParams.GetCount(); ++i)
	{
		CLoadedParam& Prm = MtlParams[i];
		U8 Type = Prm.Type;
		if (Type == Render::EPT_Const)
		{
			Render::CEffectConstant& Rec = Effect->MaterialConsts[ConstCount];
			Rec.ID = Prm.ID;
			Rec.Handle = Prm.Handle;
			Rec.BufferHandle = Prm.BufferHandle;
			Rec.ShaderType = (Render::EShaderType)Prm.ShaderType;
			Rec.pDefaultValue = NULL;
			Rec.SizeInBytes = Prm.SizeInBytes;
			++ConstCount;
			
			if (MtlConstBuffers.FindIndex(Prm.BufferHandle) == INVALID_INDEX)
				MtlConstBuffers.Add(Prm.BufferHandle);
		}
		else if (Type == Render::EPT_Resource)
		{
			Render::CEffectResource& Rec = Effect->MaterialResources[ResourceCount];
			Rec.ID = Prm.ID;
			Rec.Handle = Prm.Handle;
			Rec.ShaderType = (Render::EShaderType)Prm.ShaderType;
			++ResourceCount;
		}
		else if (Type == Render::EPT_Sampler)
		{
			Render::CEffectSampler& Rec = Effect->MaterialSamplers[SamplerCount];
			Rec.ID = Prm.ID;
			Rec.Handle = Prm.Handle;
			Rec.ShaderType = (Render::EShaderType)Prm.ShaderType;
			++SamplerCount;
		}
	}

	//???save precalc in a tool?
	Effect->MaterialConstantBufferCount = MtlConstBuffers.GetCount();
	MtlConstBuffers.Clear(true);

	if (!LoadEffectParamValues(Reader, *Effect.GetUnsafe(), GPU)) return NULL;

	Effect->BeginAddTechs(Techs.GetCount());
	for (UPTR i = 0; i < Techs.GetCount(); ++i)
		Effect->AddTech(Techs.ValueAt(i));
	Effect->EndAddTechs();

	return Effect.GetUnsafe();
}
//---------------------------------------------------------------------

// Out array will be sorted by ID as parameters are saved sorted by ID
bool CEffectLoader::LoadEffectParams(IO::CBinaryReader& Reader, Render::PShaderLibrary ShaderLibrary, CArray<CLoadedParam>& Out)
{
	//???use .shd / .csh for all?
	const char* pExtension[] = { ".vsh", ".psh", ".gsh", ".hsh", ".dsh" };

	U32 ParamCount;
	if (!Reader.Read<U32>(ParamCount)) FAIL;
	for (UPTR ParamIdx = 0; ParamIdx < ParamCount; ++ParamIdx)
	{
		CStrID ParamID;
		if (!Reader.Read(ParamID)) FAIL;

		U8 Type;
		if (!Reader.Read(Type)) FAIL;

		U8 ShaderType;
		if (!Reader.Read(ShaderType)) FAIL;

		U32 SourceShaderID;
		if (!Reader.Read(SourceShaderID)) FAIL;

		U8 ConstType;
		U32 SizeInBytes;
		if (Type == Render::EPT_Const)
		{
			if (!Reader.Read(ConstType)) FAIL;
			if (!Reader.Read(SizeInBytes)) FAIL;
		}

		Render::PShader ParamShader = ShaderLibrary->GetShaderByID(SourceShaderID);

		HHandle hParam = INVALID_HANDLE;
		switch (Type)
		{
			case Render::EPT_Const:		hParam = ParamShader->GetConstHandle(ParamID); break;
			case Render::EPT_Resource:	hParam = ParamShader->GetResourceHandle(ParamID); break;
			case Render::EPT_Sampler:	hParam = ParamShader->GetSamplerHandle(ParamID); break;
			case Render::EPT_Invalid:	FAIL;
		}

		CLoadedParam& Prm = *Out.Add();
		Prm.ID = ParamID;
		Prm.Type = Type;
		Prm.ShaderType = ShaderType;
		Prm.Handle = hParam;
		if (Type == Render::EPT_Const)
		{
			Prm.BufferHandle = ParamShader->GetConstBufferHandle(hParam);
			Prm.ConstType = ConstType;
			Prm.SizeInBytes = SizeInBytes;
		}
	}

	OK;
}
//---------------------------------------------------------------------

bool CEffectLoader::LoadEffectParamValues(IO::CBinaryReader& Reader, Render::CEffect& Effect, Render::PGPUDriver GPU)
{
	CStrID ConstWithOffset0; // A way to distinguish between NULL (no default) and default with offset = 0

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

				//!!!need binary search in fixed arrays!
				for (UPTR i = 0; i < Effect.MaterialConsts.GetCount(); ++i)
				{
					Render::CEffectConstant& Rec = Effect.MaterialConsts[i];
					if (Rec.ID == ParamID)
					{
						Rec.pDefaultValue = (void*)Offset;
						if (!Offset) ConstWithOffset0 = ParamID;
					}
				}

				break;
			}
			case Render::EPT_Resource:
			{
				Render::PTexture Texture = LoadTextureValue(Reader, GPU);
				if (Texture.IsNullPtr()) FAIL;

				//!!!need binary search in fixed arrays!
				for (UPTR i = 0; i < Effect.MaterialResources.GetCount(); ++i)
				{
					Render::CEffectResource& Rec = Effect.MaterialResources[i];
					if (Rec.ID == ParamID) Rec.DefaultValue = Texture;
				}

				break;
			}
			case Render::EPT_Sampler:
			{
				Render::PSampler Sampler = LoadSamplerValue(Reader, GPU);
				if (Sampler.IsNullPtr()) FAIL;
				
				//!!!need binary search in fixed arrays!
				for (UPTR i = 0; i < Effect.MaterialSamplers.GetCount(); ++i)
				{
					Render::CEffectSampler& Rec = Effect.MaterialSamplers[i];
					if (Rec.ID == ParamID) Rec.DefaultValue = Sampler;
				}
						
				break;
			}
		}
	}

	U32 DefValsSize;
	if (!Reader.Read(DefValsSize)) FAIL;
	if (DefValsSize)
	{
		Effect.pMaterialConstDefaultValues = (char*)n_malloc(DefValsSize);
		Reader.GetStream().Read(Effect.pMaterialConstDefaultValues, DefValsSize);
		//???store DefValsSize in CEffect?

		//!!!need binary search in fixed arrays!
		for (UPTR i = 0; i < Effect.MaterialConsts.GetCount(); ++i)
		{
			Render::CEffectConstant& Rec = Effect.MaterialConsts[i];
			if (Rec.ID == ConstWithOffset0) Rec.pDefaultValue = Effect.pMaterialConstDefaultValues;
			else if (Rec.pDefaultValue) Rec.pDefaultValue = Effect.pMaterialConstDefaultValues + (U32)Rec.pDefaultValue;
		}
	}

	OK;
}
//---------------------------------------------------------------------

bool CEffectLoader::SkipEffectParams(IO::CBinaryReader& Reader)
{
	U32 Count;
	if (!Reader.Read(Count)) FAIL;
	for (U32 i = 0; i < Count; ++i)
	{
		CString StrValue;
		U8 Type;
		if (!Reader.Read(StrValue)) FAIL;
		if (!Reader.Read(Type)) FAIL;	
		if (!Reader.GetStream().Seek(Type == 0 ? 10 : 5, IO::Seek_Current)) FAIL;
	}

	OK;
}
//---------------------------------------------------------------------

Render::PTexture CEffectLoader::LoadTextureValue(IO::CBinaryReader& Reader, Render::PGPUDriver GPU)
{
	CStrID ResourceID;
	if (!Reader.Read(ResourceID)) return NULL;

	Resources::PResource RTexture = ResourceMgr->RegisterResource(ResourceID.CStr());
	if (!RTexture->IsLoaded())
	{
		Resources::PResourceLoader Loader = RTexture->GetLoader();
		if (Loader.IsNullPtr())
			Loader = ResourceMgr->CreateDefaultLoaderFor<Render::CTexture>(PathUtils::GetExtension(ResourceID.CStr()));
		if (Loader.IsNullPtr()) return NULL;
		Loader->As<Resources::CTextureLoader>()->GPU = GPU;
		ResourceMgr->LoadResourceSync(*RTexture, *Loader);
		if (!RTexture->IsLoaded()) return NULL;
	}

	return RTexture->GetObject<Render::CTexture>();
}
//---------------------------------------------------------------------

Render::PSampler CEffectLoader::LoadSamplerValue(IO::CBinaryReader& Reader, Render::PGPUDriver GPU)
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

	return GPU->CreateSampler(SamplerDesc);
}
//---------------------------------------------------------------------

}