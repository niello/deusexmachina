#include "GraphicsResourceManager.h"
#include <Render/GPUDriver.h>
#include <Render/VertexLayout.h>
#include <Render/Texture.h>
#include <Render/TextureData.h>
#include <Render/Shader.h>
#include <Render/ShaderLibrary.h>
#include <Render/Effect.h>
#include <Render/Material.h>
#include <Render/Mesh.h>
#include <Render/MeshData.h>
#include <Resources/ResourceManager.h>
#include <Resources/Resource.h>
#include <IO/Stream.h>
#include <IO/BinaryReader.h>
#include <Data/RAMData.h>
#include <Data/StringUtils.h>
#include <map>

namespace Frame
{

CGraphicsResourceManager::CGraphicsResourceManager(Resources::CResourceManager& ResMgr, Render::CGPUDriver& GPU)
	: pResMgr(&ResMgr)
	, pGPU(&GPU)
{
}
//---------------------------------------------------------------------

CGraphicsResourceManager::~CGraphicsResourceManager() {}
//---------------------------------------------------------------------

Render::PMesh CGraphicsResourceManager::GetMesh(CStrID UID)
{
	auto It = Meshes.find(UID);
	if (It != Meshes.cend() && It->second) return It->second;

	if (!pResMgr || !pGPU) return nullptr;

	Resources::PResource RMeshData = pResMgr->RegisterResource<Render::CMeshData>(UID);
	Render::PMeshData MeshData = RMeshData->ValidateObject<Render::CMeshData>();

	if (!MeshData->UseRAMData()) return nullptr;

	//!!!Now all VBs and IBs are not shared! later this may change!

	Render::PVertexLayout VertexLayout = pGPU->CreateVertexLayout(&MeshData->VertexFormat.Front(), MeshData->VertexFormat.GetCount());
	Render::PVertexBuffer VB = pGPU->CreateVertexBuffer(*VertexLayout, MeshData->VertexCount, Render::Access_GPU_Read, MeshData->VBData->GetPtr());
	Render::PIndexBuffer IB;
	if (MeshData->IndexCount)
		IB = pGPU->CreateIndexBuffer(MeshData->IndexType, MeshData->IndexCount, Render::Access_GPU_Read, MeshData->IBData->GetPtr());

	Render::PMesh Mesh = n_new(Render::CMesh);
	const bool Result = Mesh->Create(MeshData, VB, IB);

	MeshData->ReleaseRAMData();

	if (!Result) return nullptr;

	Meshes.emplace(UID, Mesh);

	return Mesh;
}
//---------------------------------------------------------------------

Render::PTexture CGraphicsResourceManager::GetTexture(CStrID UID, UPTR AccessFlags)
{
	auto It = Textures.find(UID);
	if (It != Textures.cend() && It->second)
	{
		n_assert(It->second->GetAccess().Is(AccessFlags));
		return It->second;
	}

	if (!pResMgr || !pGPU) return nullptr;

	Resources::PResource RTexData = pResMgr->RegisterResource<Render::CTextureData>(UID);
	Render::PTextureData TexData = RTexData->ValidateObject<Render::CTextureData>();

	if (!TexData->UseRAMData()) return nullptr;

	Render::PTexture Texture = pGPU->CreateTexture(TexData, AccessFlags);

	TexData->ReleaseRAMData();

	if (Texture) Textures.emplace(UID, Texture);

	return Texture;
}
//---------------------------------------------------------------------

Render::PShader CGraphicsResourceManager::GetShader(CStrID UID)
{
	auto It = Shaders.find(UID);
	if (It != Shaders.cend() && It->second) return It->second;

	// Tough resource manager doesn't keep track of shaders, it is used
	// for shader library loading and for accessing an IO service.
	if (!pResMgr || !pGPU) return nullptr;

	IO::PStream Stream;
	Render::PShaderLibrary ShaderLibrary;
	const char* pSubId = strchr(UID.CStr(), '#');
	if (pSubId)
	{
		// Generated shaders are not supported, must be a file
		if (pSubId == UID.CStr()) return nullptr;

		// File must be a ShaderLibrary if sub-ID is used
		CString Path(UID.CStr(), pSubId - UID.CStr());
		++pSubId; // Skip '#'
		if (*pSubId == 0) return nullptr;

		Resources::PResource RShaderLibrary = pResMgr->RegisterResource<Render::CShaderLibrary>(CStrID(Path));
		if (!RShaderLibrary) return nullptr;

		ShaderLibrary = RShaderLibrary->ValidateObject<Render::CShaderLibrary>();
		if (!ShaderLibrary) return nullptr;

		const U32 ElementID = StringUtils::ToInt(pSubId);
		Stream = ShaderLibrary->GetElementStream(ElementID);
	}
	else Stream = pResMgr->CreateResourceStream(UID, pSubId);

	if (!Stream || !Stream->Open(IO::SAM_READ, IO::SAP_SEQUENTIAL) || !Stream->CanRead()) return nullptr;

	Render::PShader Shader = pGPU->CreateShader(*Stream, ShaderLibrary.Get());

	if (Shader) Shaders.emplace(UID, Shader);

	return Shader;
}
//---------------------------------------------------------------------

Render::PEffect CGraphicsResourceManager::GetEffect(CStrID UID)
{
	auto It = Effects.find(UID);
	if (It != Effects.cend() && It->second) return It->second;

	Render::PEffect Effect = LoadEffect(UID);
	if (!Effect) return nullptr;

	Effects.emplace(UID, Effect);

	return Effect;
}
//---------------------------------------------------------------------

Render::PMaterial CGraphicsResourceManager::GetMaterial(CStrID UID)
{
	auto It = Materials.find(UID);
	if (It != Materials.cend() && It->second) return It->second;

	Render::PMaterial Material = LoadMaterial(UID);
	if (!Material) return nullptr;

	Materials.emplace(UID, Material);

	return Material;
}
//---------------------------------------------------------------------

PRenderPath CGraphicsResourceManager::GetRenderPath(CStrID UID)
{
	auto It = RenderPaths.find(UID);
	if (It != RenderPaths.cend() && It->second) return It->second;

	PRenderPath RP = LoadRenderPath(UID);
	if (!RP) return nullptr;

	RenderPaths.emplace(UID, RP);

	return RP;
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

Render::PEffect CGraphicsResourceManager::LoadEffect(CStrID UID)
{
	if (!pResMgr || !pGPU) return nullptr;

	const char* pOutSubId;
	IO::PStream Stream = pResMgr->CreateResourceStream(UID, pOutSubId);
	if (!Stream || !Stream->Open(IO::SAM_READ, IO::SAP_SEQUENTIAL) || !Stream->CanRead()) return nullptr;

	IO::CBinaryReader Reader(*Stream);

	U32 Magic;
	if (!Reader.Read<U32>(Magic) || Magic != 'SHFX') FAIL;

	U32 Version;
	if (!Reader.Read<U32>(Version) || Version > 0x00010000) FAIL;

	CStrID MaterialType;
	if (!Reader.Read(MaterialType)) FAIL;

	U32 ShaderFormatCount;
	if (!Reader.Read<U32>(ShaderFormatCount) || !ShaderFormatCount) FAIL;

	std::map<U32, U32> Offsets;
	for (U32 i = 0; i < ShaderFormatCount; ++i)
	{
		U32 Format;
		if (!Reader.Read(Format)) FAIL;
		U32 Offset;
		if (!Reader.Read(Offset)) FAIL;

		if (pGPU->SupportsShaderFormat(Format))
			Offsets.emplace(Format, Offset);
	}

	for (const auto& Pair : Offsets)
	{
		Reader.GetStream().Seek(Pair.second, IO::Seek_Begin);

		// read global param table
		// read material param table

		//???universal?
		// read default value count
		// read material defaults
		// read const value buffer for defaults

		// tech count

		//!!!add offet for skipping when tech feature level is unacceptable!
		// techs:
		// -feature level u32
		// -offset of the next tech u32
		// -input set str
		// -pass count u32
		// -pass indices array of u32
		// -tech param table

		// render state count u32
		// render states
	}

	FAIL;


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
					*pShaders[ShaderType] = nullptr;
					continue;
				}

				//!!!DBG TMP! store full resource ID instead
				CStrID SUID = CStrID("ShLib:#" + StringUtils::FromInt(ShaderID));

				*pShaders[ShaderType] = GPU.GetShader(SUID);
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
		const Render::HConstantBuffer hCB = MaterialConsts[i].Const->GetConstantBufferHandle();
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

Render::PMaterial CGraphicsResourceManager::LoadMaterial(CStrID UID)
{
	if (!pResMgr || !pGPU) return nullptr;

	const char* pOutSubId;
	IO::PStream Stream = pResMgr->CreateResourceStream(UID, pOutSubId);
	if (!Stream || !Stream->Open(IO::SAM_READ, IO::SAP_SEQUENTIAL) || !Stream->CanRead()) return nullptr;

	IO::CBinaryReader Reader(*Stream);

	U32 Magic;
	if (!Reader.Read<U32>(Magic) || Magic != 'MTRL') FAIL;

	U32 Version;
	if (!Reader.Read<U32>(Version)) FAIL;

	CStrID EffectID;
	if (!Reader.Read(EffectID)) FAIL;
	if (!EffectID.IsValid()) FAIL;

	Effect = GetEffect(EffectID);

	// Build parameters

	CDict<CStrID, void*>			ConstValues;
	CDict<CStrID, Render::PTexture>	ResourceValues;
	CDict<CStrID, Render::PSampler>	SamplerValues;
	void*							pConstValueBuffer;	// Must be n_free()'d if not nullptr

	if (!CEffect::LoadParamValues(Reader, GPU, ConstValues, ResourceValues, SamplerValues, pConstValueBuffer)) FAIL;

	const CFixedArray<Render::CEffectConstant>& Consts = Effect->GetMaterialConstants();
	ConstBuffers.SetSize(Effect->GetMaterialConstantBufferCount());
	UPTR CurrCBCount = 0;
	for (UPTR i = 0; i < Consts.GetCount(); ++i)
	{
		const Render::CEffectConstant& Const = Consts[i];

		const Render::HConstantBuffer hCB = Const.Const->GetConstantBufferHandle();
		Render::CMaterial::CConstBufferRecord* pRec = nullptr;
		for (UPTR BufIdx = 0; BufIdx < CurrCBCount; ++BufIdx)
			if (ConstBuffers[BufIdx].Handle == hCB)
			{
				pRec = &ConstBuffers[BufIdx];
				break;
			}

		if (!pRec)
		{
			pRec = &ConstBuffers[CurrCBCount];
			pRec->Handle = hCB;
			pRec->ShaderType = Const.ShaderType;
			pRec->Buffer = GPU.CreateConstantBuffer(hCB, Render::Access_CPU_Write); //!!!must be a RAM-only buffer!
			++CurrCBCount;

			if (!GPU.BeginShaderConstants(*pRec->Buffer.Get())) FAIL;
		}

		IPTR Idx = ConstValues.FindIndex(Const.ID);
		void* pValue = (Idx != INVALID_INDEX) ? ConstValues.ValueAt(Idx) : nullptr;
		if (!pValue) pValue = Effect->GetConstantDefaultValue(Const.ID);

		if (pValue) //???fail if value is undefined? or fill with zeroes?
		{
			if (Const.Const.IsNullPtr()) FAIL;
			Const.Const->SetRawValue(*pRec->Buffer.Get(), pValue, Render::CShaderConstant::WholeSize);
		}
	}

	SAFE_FREE(pConstValueBuffer);

	for (UPTR BufIdx = 0; BufIdx < CurrCBCount; ++BufIdx)
	{
		Render::CMaterial::CConstBufferRecord* pRec = &ConstBuffers[BufIdx];
		Render::PConstantBuffer RAMBuffer = pRec->Buffer;
		if (!GPU.CommitShaderConstants(*RAMBuffer.Get())) FAIL; //!!!must not do any VRAM operations inside!

																//???do only if current buffer doesn't support VRAM? DX9 will support, DX11 will not.
																//if supports VRAM, can reuse as VRAM buffer without data copying between RAMBuffer and a new one.
		pRec->Buffer = GPU.CreateConstantBuffer(pRec->Handle, Render::Access_GPU_Read, RAMBuffer.Get());
	}

	const CFixedArray<Render::CEffectResource>& EffectResources = Effect->GetMaterialResources();
	Resources.SetSize(EffectResources.GetCount());
	for (UPTR i = 0; i < EffectResources.GetCount(); ++i)
	{
		const Render::CEffectResource& Rsrc = EffectResources[i];

		IPTR Idx = ResourceValues.FindIndex(Rsrc.ID);
		Render::PTexture Value = (Idx != INVALID_INDEX) ? ResourceValues.ValueAt(Idx) : Render::PTexture();

		Render::CMaterial::CResourceRecord& Rec = Resources[i];
		Rec.Handle = Rsrc.Handle;
		Rec.ShaderType = Rsrc.ShaderType;
		Rec.Resource = Value.IsValidPtr() ? Value : Effect->GetResourceDefaultValue(Rsrc.ID);
	}

	const CFixedArray<Render::CEffectSampler>& EffectSamplers = Effect->GetMaterialSamplers();
	Samplers.SetSize(EffectSamplers.GetCount());
	for (UPTR i = 0; i < EffectSamplers.GetCount(); ++i)
	{
		const Render::CEffectSampler& Sampler = EffectSamplers[i];

		IPTR Idx = SamplerValues.FindIndex(Sampler.ID);
		Render::PSampler Value = (Idx != INVALID_INDEX) ? SamplerValues.ValueAt(Idx) : Render::PSampler();

		Render::CMaterial::CSamplerRecord& Rec = Samplers[i];
		Rec.Handle = Sampler.Handle;
		Rec.ShaderType = Sampler.ShaderType;
		Rec.Sampler = Value.IsValidPtr() ? Value : Effect->GetSamplerDefaultValue(Sampler.ID);
	}

	Render::PMaterial Material = n_new(Render::CMaterial);

	OK;
}
//---------------------------------------------------------------------

PRenderPath CGraphicsResourceManager::LoadRenderPath(CStrID UID)
{
	if (!pResMgr || !pGPU) return nullptr;

	const char* pOutSubId;
	IO::PStream Stream = pResMgr->CreateResourceStream(UID, pOutSubId);
	if (!Stream || !Stream->Open(IO::SAM_READ, IO::SAP_SEQUENTIAL) || !Stream->CanRead()) return nullptr;

	IO::CBinaryReader Reader(*Stream);

	U32 Magic;
	if (!Reader.Read<U32>(Magic) || Magic != 'RPTH') return nullptr;

	U32 Version;
	if (!Reader.Read<U32>(Version)) return nullptr;

	U32 ShaderModel;
	if (!Reader.Read<U32>(ShaderModel)) return nullptr; // 0 for SM3.0, 1 for USM

	Data::CDataArray RTSlots;
	if (!Reader.Read(RTSlots)) return nullptr;

	Data::CDataArray DSSlots;
	if (!Reader.Read(DSSlots)) return nullptr;

	Data::PParams Phases = n_new(Data::CParams);
	if (!Reader.ReadParams(*Phases)) return nullptr;

	Frame::PRenderPath RP = n_new(Frame::CRenderPath);

	RP->RTSlots.SetSize(RTSlots.GetCount());
	for (UPTR i = 0; i < RTSlots.GetCount(); ++i)
	{
		Frame::CRenderPath::CRenderTargetSlot& Slot = RP->RTSlots[i];
		Slot.ClearValue = RTSlots[i].GetValue<Data::PParams>()->Get<vector4>(CStrID("ClearValue"), vector4(0.5f, 0.5f, 0.f, 1.f));
	}

	RP->DSSlots.SetSize(DSSlots.GetCount());
	for (UPTR i = 0; i < DSSlots.GetCount(); ++i)
	{
		Frame::CRenderPath::CDepthStencilSlot& Slot = RP->DSSlots[i];
		Data::PParams SlotParams = DSSlots[i].GetValue<Data::PParams>();

		Slot.ClearFlags = 0;

		float ZClear;
		if (SlotParams->Get(ZClear, CStrID("DepthClearValue")))
		{
			Slot.DepthClearValue = ZClear;
			Slot.ClearFlags |= Render::Clear_Depth;
		}

		int StencilClear;
		if (SlotParams->Get(StencilClear, CStrID("StencilClearValue")))
		{
			Slot.StencilClearValue = StencilClear;
			Slot.ClearFlags |= Render::Clear_Stencil;
		}
	}

	// Breaks API independence. Honestly, RP file format breaks it even earlier,
	// by including API-specific metadata. Subject to redesign.
	//???load separate metadata-only shader file by GPU?
	switch (ShaderModel)
	{
		case 0: // SM3.0
		{
			Render::CSM30ShaderMetadata* pGlobals = n_new(Render::CSM30ShaderMetadata);
			if (!pGlobals->Load(*Stream)) return nullptr;
			RP->pGlobals = pGlobals;
			break;
		}
		case 1: // USM
		{
			Render::CUSMShaderMetadata* pGlobals = n_new(Render::CUSMShaderMetadata);
			if (!pGlobals->Load(*Stream)) return nullptr;
			RP->pGlobals = pGlobals;
			break;
		}
	}

	if (!LoadGlobalParams(Reader, RP->pGlobals, RP->Consts, RP->Resources, RP->Samplers)) return nullptr;

	// Phases are intentionally initialized at the end, because they may access global params etc
	RP->Phases.SetSize(Phases->GetCount());

	for (UPTR i = 0; i < Phases->GetCount(); ++i)
	{
		const Data::CParam& Prm = Phases->Get(i);
		Data::CParams& PhaseDesc = *Prm.GetValue<Data::PParams>();

		const CString& PhaseType = PhaseDesc.Get<CString>(CStrID("Type"), CString::Empty); //???use FourCC in PRM?
		CString ClassName = "Frame::CRenderPhase" + PhaseType;
		Frame::PRenderPhase CurrPhase = (Frame::CRenderPhase*)Factory->Create(ClassName.CStr());

		if (!CurrPhase->Init(*RP.Get(), Prm.GetName(), PhaseDesc)) return nullptr;

		RP->Phases[i] = CurrPhase;
	}

	return RP.Get();
}
//---------------------------------------------------------------------


///////////

// Almost the same as CEffect::LoadParams but doesn't depend on GPU
// Out array will be sorted by ID as parameters are saved sorted by ID
bool CGraphicsResourceManager::LoadGlobalParams(IO::CBinaryReader& Reader,
	const Render::IShaderMetadata* pShaderMeta,
	CFixedArray<Render::CEffectConstant>& OutConsts,
	CFixedArray<Render::CEffectResource>& OutResources,
	CFixedArray<Render::CEffectSampler>& OutSamplers)
{
	if (!pShaderMeta) FAIL;

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

		Render::HConstant hConst = pShaderMeta->GetConstantHandle(ParamID);
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

		Render::CEffectSampler& Rec = OutSamplers[ParamIdx];
		Rec.ID = ParamID;
		Rec.Handle = pShaderMeta->GetSamplerHandle(ParamID);
		Rec.ShaderType = (Render::EShaderType)ShaderType;
	}

	OK;
}
//---------------------------------------------------------------------

// Out array will be sorted by ID as parameters are saved sorted by ID
bool CGraphicsResourceManager::LoadEffectParams(IO::CBinaryReader& Reader,
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
		}
		if (!pShaderMeta) FAIL;

		Render::HConstant hConst = pShaderMeta->GetConstantHandle(ParamID);
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

bool CGraphicsResourceManager::LoadEffectParamValues(IO::CBinaryReader& Reader,
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
				Render::PTexture Texture = GetTexture(RUID, Render::Access_GPU_Read);
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
	else pOutConstValueBuffer = nullptr;

	OK;
}
//---------------------------------------------------------------------


}
