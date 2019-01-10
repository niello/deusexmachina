#include "EffectLoader.h"

#include <Render/GPUDriver.h>
#include <Render/Effect.h>
#include <Render/Shader.h>
#include <Render/ShaderConstant.h>
#include <Render/RenderStateDesc.h>
#include <Render/ShaderLibrary.h>
#include <IO/BinaryReader.h>

namespace Resources
{
// Defined in Render/EffectLoadingUtils.cpp
bool LoadEffectParams(IO::CBinaryReader& Reader, Render::PShaderLibrary ShaderLibrary, const Render::IShaderMetadata* pDefaultShaderMeta, CFixedArray<Render::CEffectConstant>& OutConsts, CFixedArray<Render::CEffectResource>& OutResources, CFixedArray<Render::CEffectSampler>& OutSamplers);
bool LoadEffectParamValues(IO::CBinaryReader& Reader, Render::PGPUDriver GPU, CDict<CStrID, void*>& OutConsts, CDict<CStrID, Render::PTexture>& OutResources, CDict<CStrID, Render::PSampler>& OutSamplers, void*& pOutConstValueBuffer);
bool SkipEffectParams(IO::CBinaryReader& Reader);

CEffectLoader::~CEffectLoader() {}

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

	U32 ShaderModel;
	if (!Reader.Read<U32>(ShaderModel)) return NULL; // 0 for SM3.0, 1 for USM

	U32 EffectType;
	if (!Reader.Read<U32>(EffectType)) return NULL;

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

			Render::CRenderStateDesc::CRTBlend& RTBlend = Desc.RTBlend[BlendIdx];

			if (!Reader.Read(RTBlend.WriteMask)) return NULL;

			if (Desc.Flags.IsNot(Render::CRenderStateDesc::Blend_RTBlendEnable << BlendIdx)) continue;

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

		if (!LoadEffectParams(Reader, ShaderLibrary, NULL, Tech->Consts, Tech->Resources, Tech->Samplers)) return NULL;
	}

	if (!Techs.GetCount()) return NULL;

	//!!!try to find by effect ID! add into it, if found!
	//!!!now rsrc mgmt doesn't support miltiple resources per file and partial resource definitions (multiple files per resource)!
	//may modify CEffectLoader to store all files of the effect, and use effect ID and rsrc URI separately.
	Render::PEffect Effect = n_new(Render::CEffect);

	switch (EffectType)
	{
		case 0:		Effect->Type = Render::EffectType_Opaque; break;
		case 1:		Effect->Type = Render::EffectType_AlphaTest; break;
		case 2:		Effect->Type = Render::EffectType_Skybox; break;
		case 3:		Effect->Type = Render::EffectType_AlphaBlend; break;
		default:	Effect->Type = Render::EffectType_Other; break;
	}

	// Skip global params

	if (!SkipEffectParams(Reader)) return NULL;

	// Load material params

	if (!LoadEffectParams(Reader, ShaderLibrary, NULL, Effect->MaterialConsts, Effect->MaterialResources, Effect->MaterialSamplers)) return NULL;

	//???save precalc in a tool?
	CArray<HHandle> MtlConstBuffers;
	for (UPTR i = 0; i < Effect->MaterialConsts.GetCount(); ++i)
	{
		const Render::HConstBuffer hCB = Effect->MaterialConsts[i].Const->GetConstantBufferHandle();
		if (!MtlConstBuffers.Contains(hCB)) MtlConstBuffers.Add(hCB);
	}
	Effect->MaterialConstantBufferCount = MtlConstBuffers.GetCount();
	MtlConstBuffers.Clear(true);

	void* pVoidBuffer;
	if (!LoadEffectParamValues(Reader, GPU, Effect->DefaultConsts, Effect->DefaultResources, Effect->DefaultSamplers, pVoidBuffer)) return NULL;
	Effect->pMaterialConstDefaultValues = (char*)pVoidBuffer;

	Effect->BeginAddTechs(Techs.GetCount());
	for (UPTR i = 0; i < Techs.GetCount(); ++i)
		Effect->AddTech(Techs.ValueAt(i));
	Effect->EndAddTechs();

	return Effect.GetUnsafe();
}
//---------------------------------------------------------------------

}