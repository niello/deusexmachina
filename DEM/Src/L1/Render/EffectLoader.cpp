#include "EffectLoader.h"

#include <Render/GPUDriver.h>
#include <Render/Effect.h>
#include <Render/RenderStateDesc.h>
#include <Render/SamplerDesc.h>
#include <Render/Texture.h>
#include <Render/Shader.h>
#include <Render/ShaderLoader.h>
#include <Render/RenderFwd.h>
#include <Resources/Resource.h>
#include <Resources/ResourceManager.h>
#include <IO/IOServer.h>
#include <IO/BinaryReader.h>
#include <IO/PathUtils.h>
#include <Data/StringUtils.h>

namespace Resources
{

enum EEffectParamType
{
	EPT_Const		= 0,
	EPT_Resource	= 1,
	EPT_Sampler		= 2,

	EPT_Invalid
};

const Core::CRTTI& CEffectLoader::GetResultType() const
{
	return Render::CEffect::RTTI;
}
//---------------------------------------------------------------------

//!!!can sort techs by InputID from best to worst! also can first load techs, then RS, not to load RS that won't be used!
bool CEffectLoader::Load(CResource& Resource)
{
	if (GPU.IsNullPtr()) FAIL;

	const char* pURI = Resource.GetUID().CStr();
	IO::PStream File = IOSrv->CreateStream(pURI);
	if (!File->Open(IO::SAM_READ, IO::SAP_SEQUENTIAL)) FAIL;
	IO::CBinaryReader Reader(*File);

	U32 Magic;
	if (!Reader.Read<U32>(Magic) || Magic != 'SHFX') FAIL;

	U32 Version;
	if (!Reader.Read<U32>(Version)) FAIL;

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
			if (Desc.Flags.IsNot(Render::CRenderStateDesc::Blend_RTBlendEnable << BlendIdx)) continue;

			Render::CRenderStateDesc::CRTBlend& RTBlend = Desc.RTBlend[BlendIdx];

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

			if (!Reader.Read(RTBlend.WriteMask)) FAIL;
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

			//???use .shd / .csh for all?
			const char* pExtension[] = { ".vsh", ".psh", ".gsh", ".hsh", ".dsh" };
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

				CString URI = "Shaders:Bin/" + StringUtils::FromInt(ShaderID) + pExtension[ShaderType];
				Resources::PResource RShader = ResourceMgr->RegisterResource(URI.CStr());
				if (!RShader->IsLoaded())
				{
					Resources::PResourceLoader Loader = RShader->GetLoader();
					if (Loader.IsNullPtr())
						Loader = ResourceMgr->CreateDefaultLoaderFor<Render::CShader>(pExtension[ShaderType] + 1); // +1 to skip dot
					Loader->As<Resources::CShaderLoader>()->GPU = GPU;
					ResourceMgr->LoadResourceSync(*RShader, *Loader);
					if (!RShader->IsLoaded())
					{
						ShaderLoadingFailed = true;
						break;
					}
				}

				*pShaders[ShaderType] = RShader->GetObject<Render::CShader>();
			}

			Variations[LightCount] = ShaderLoadingFailed ? NULL : GPU->CreateRenderState(Desc);
			if (Variations[LightCount].IsValidPtr()) VariationArraySize = LightCount + 1;
		}

		if (VariationArraySize < LightVariationCount)
			Variations.SetSize(VariationArraySize, true);
	}

	CDict<CStrID, Render::PTechnique> Techs;

	Render::EGPUFeatureLevel GPULevel = GPU->GetFeatureLevel();

	U32 TechCount;
	if (!Reader.Read<U32>(TechCount) || !TechCount) FAIL;
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

		// Check for hardware and API support
		if (MinFeatureLevel > GPULevel) continue;
		if (!GPU->SupportsShaderModel(Target)) continue;
		//!!!check RequiresFlags!

		// Check for already loaded tech, isn't it better. We intentionally keep only
		// one tech per InputSet loaded. Someone may want to load more techs for a
		// particular InputSet and choose by name, but it is considered a rare case
		// for a game. For low-performance graphic cards with rich feature support,
		// a better way is to measure GPU performance on driver init and choose here
		// not only by features supported, but by performance 'score' of tech vs GPU too.
		//!!!sort techs from the best to the lightest, to avoid loading and then discarding!
		Render::PTechnique Tech;
		IPTR TechDictIdx = Techs.FindIndex(InputSet);
		if (TechDictIdx == INVALID_INDEX)
		{
			Tech = n_new(Render::CTechnique);
			Tech->Name = TechID;
			Tech->ShaderInputSetID = Render::RegisterShaderInputSetID(InputSet);
			Tech->MinFeatureLevel = MinFeatureLevel;
		}
		else
		{
			Tech = Techs.ValueAt(TechDictIdx);
			if (Tech->MinFeatureLevel > MinFeatureLevel) continue;
		}

		bool IsValidTech = true;

		U32 PassCount;
		if (!Reader.Read<U32>(PassCount)) FAIL;
		CFixedArray<U32> PassRenderStateIndices(PassCount);
		for (UPTR PassIdx = 0; PassIdx < PassCount; ++PassIdx)
		{
			U32 PassRenderStateIdx;
			if (!Reader.Read<U32>(PassRenderStateIdx)) FAIL;

			//!!!if tech is loaded with errors (some variations are discarded on load), leave it loaded,
			//but continue loading techs for this input set, to allow more lightweight tech possibly
			//defined later to implement all variations
			if (PassRenderStateIdx == INVALID_INDEX || !RenderStates[PassRenderStateIdx].GetCount())
			{
				IsValidTech = false;
				break;
			}
			
			PassRenderStateIndices[PassIdx] = PassRenderStateIdx;
		}

		if (!IsValidTech) continue;

		U32 MaxLights;
		if (!Reader.Read<U32>(MaxLights)) FAIL;

		// Truncate to ensure VariationValid flags will suit into an U32 flags
		// This limitation can easily be avoided by using an array of bool, but
		// in is doubtful that anyone will try to render more than 31 forward light.
		//if (MaxLights > 31) MaxLights = 31;

		//U32 LightVariationValidityFlags = 0;
		UPTR NewVariationCount = 0;
		UPTR LightVariationCount = MaxLights + 1;

		Tech->PassesByLightCount.SetSize(LightVariationCount);

		for (UPTR LightCount = 0; LightCount < LightVariationCount; ++LightCount)
		{
			// Always 0 or 1
			U8 VariationValid;
			if (!Reader.Read<U8>(VariationValid)) FAIL;
			//LightVariationValidityFlags |= (VariationValid << LightCount);

			Render::CPassList& PassList = Tech->PassesByLightCount[LightCount];
			PassList.SetSize(PassCount);

			if (VariationValid)
			{
				for (UPTR PassIdx = 0; PassIdx < PassCount; ++PassIdx)
				{
					Render::PRenderState RS = RenderStates[PassRenderStateIndices[PassIdx]][LightCount];
					if (RS.IsNullPtr())
					{
						VariationValid = false;
						break;
					}
					PassList[PassIdx] = RS;
				}
			}

			if (VariationValid)
			{
				NewVariationCount = LightCount + 1;
			}
			else
			{
				PassList.Clear();
				// if LightCount is more than pass allows, truncate MaxLights to LightCount - 1
				// if LightCount was 0 in this case, fail tech
			}

		}

		if (!NewVariationCount) continue; // No valid variations in a tech
		else if (NewVariationCount < LightVariationCount)
			Tech->PassesByLightCount.SetSize(NewVariationCount, true);

		// Load tech params info
		
		LoadEffectParams(Reader, false);

		//if (TechInfo.Target < 0x0400)
		//{
		//	WriteRegisterRanges(TechInfo.UsedFloat4, W, "float4");
		//	WriteRegisterRanges(TechInfo.UsedInt4, W, "int4");
		//	WriteRegisterRanges(TechInfo.UsedBool, W, "bool");
		//}

		if (TechDictIdx == INVALID_INDEX) Techs.Add(InputSet, Tech);
	}

	if (!Techs.GetCount()) FAIL;

	// Load global and material params tables

	LoadEffectParams(Reader, false); // Global
	LoadEffectParams(Reader, true); // Material

	//!!!try to find by effect ID! add into it, if found!
	//!!!now rsrc mgmt doesn't support miltiple resources per file and partial resource definitions (multiple files per resource)!
	//may modify CEffectLoader to store all files of the effect, and use effect ID and rsrc URI separately.
	Render::PEffect Effect = n_new(Render::CEffect);

	Effect->BeginAddTechs(Techs.GetCount());
	for (UPTR i = 0; i < Techs.GetCount(); ++i)
		Effect->AddTech(Techs.ValueAt(i));
	Effect->EndAddTechs();

	Resource.Init(Effect.GetUnsafe(), this);

	OK;
}
//---------------------------------------------------------------------

//???save (and therefore load) default values separately? in a separate loop.
//defaults are needed only for materials, so they are strictly a load-time feature,
//no reason to store them attached to a main dictionary.
bool CEffectLoader::LoadEffectParams(IO::CBinaryReader& Reader, bool ReadDefaults) const
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

		//???use shader bank and simple indexing?
		CString URI = "Shaders:Bin/" + StringUtils::FromInt(SourceShaderID) + pExtension[ShaderType];
		Resources::PResource RShader = ResourceMgr->RegisterResource(URI.CStr());
		if (!RShader->IsLoaded()) FAIL; // Failed to find referenced shader
		Render::PShader ParamShader = RShader->GetObject<Render::CShader>();

		HHandle hParam = INVALID_HANDLE;
		switch (Type)
		{
			case EPT_Const:		hParam = ParamShader->GetConstHandle(ParamID); break;
			case EPT_Resource:	hParam = ParamShader->GetResourceHandle(ParamID); break;
			case EPT_Sampler:	hParam = ParamShader->GetSamplerHandle(ParamID); break;
			case EPT_Invalid:	FAIL;
		}

		// save ID to handle mapping, mb one dictionary per parameter type
		// ParamID -> hParam, for const: hBuffer = Effect->GetConstBufferHandle(hParam), same as in CShader

		if (ReadDefaults)
		{
			bool HasDefaultValue;
			if (!Reader.Read(HasDefaultValue)) FAIL;
			
			if (HasDefaultValue)
			{
				switch (Type)
				{
					case EPT_Const:
					{
						NOT_IMPLEMENTED;
						// get const type
						// load default value as raw bytes
						break;
					}
					case EPT_Resource:
					{
						CStrID ResourceID;
						if (!Reader.Read(ResourceID)) FAIL;

						Resources::PResource RTexture = ResourceMgr->RegisterResource(ResourceID.CStr());
						if (!RTexture->IsLoaded())
						{
							Resources::PResourceLoader Loader = RTexture->GetLoader();
							if (Loader.IsNullPtr())
								Loader = ResourceMgr->CreateDefaultLoaderFor<Render::CTexture>(PathUtils::GetExtension(ResourceID.CStr()));
							Loader->As<Resources::CShaderLoader>()->GPU = GPU;
							ResourceMgr->LoadResourceSync(*RTexture, *Loader);
							if (!RTexture->IsLoaded()) FAIL;
						}

						Render::PTexture DefaultTexture = RTexture->GetObject<Render::CTexture>();

						//!!!save it somewhere!

						break;
					}
					case EPT_Sampler:
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

						Render::PSampler DefaultSampler = GPU->CreateSampler(SamplerDesc);

						//!!!save it somewhere!
						
						break;
					}
				}
			}
		}
	}

	OK;
}
//---------------------------------------------------------------------

}