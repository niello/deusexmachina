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
#include <Render/RenderStateDesc.h>
#include <Render/SamplerDesc.h>
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

Render::PShader CGraphicsResourceManager::GetShader(CStrID UID, bool NeedParamTable)
{
	auto It = Shaders.find(UID);
	if (It != Shaders.cend() && It->second)
	{
		if (NeedParamTable && !It->second->GetParamTable())
		{
			// TODO: load and set param table to the existing shader
			NOT_IMPLEMENTED;
		}

		return It->second;
	}

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

	Render::PShader Shader = pGPU->CreateShader(*Stream, ShaderLibrary.Get(), NeedParamTable);

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

bool CGraphicsResourceManager::LoadShaderParamValues(IO::CBinaryReader& Reader, Render::CShaderParamValues& Out)
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
				Out.ConstValues.emplace(ParamID, (void*)Offset);
				break;
			}
			case Render::EPT_Resource:
			{
				CStrID RUID;
				if (!Reader.Read(RUID)) FAIL;
				Render::PTexture Texture = GetTexture(RUID, Render::Access_GPU_Read);
				if (Texture.IsNullPtr()) FAIL;
				Out.ResourceValues.emplace(ParamID, Texture);
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

				Render::PSampler Sampler = pGPU->CreateSampler(SamplerDesc);
				if (Sampler.IsNullPtr()) FAIL;
				Out.SamplerValues.emplace(ParamID, Sampler);
				break;
			}
		}
	}

	U32 ValueBufferSize;
	if (!Reader.Read(ValueBufferSize)) FAIL;
	if (ValueBufferSize)
	{
		Out.ConstValueBuffer = std::make_unique<char[]>(ValueBufferSize);
		Reader.GetStream().Read(Out.ConstValueBuffer.get(), ValueBufferSize);

		// Covert offsets to direct pointers
		for (auto& Pair : Out.ConstValues)
			Pair.second = Out.ConstValueBuffer.get() + (U32)Pair.second;
	}

	OK;
}
//---------------------------------------------------------------------

bool CGraphicsResourceManager::LoadRenderStateDesc(IO::CBinaryReader& Reader, Render::CRenderStateDesc& Out)
{
	Out.SetDefaults();

	//U32 MaxLights;
	//if (!Reader.Read(MaxLights)) FAIL;
	//UPTR LightVariationCount = MaxLights + 1;

	U8 U8Value;
	U32 U32Value;

	if (!Reader.Read(U32Value)) FAIL;
	Out.Flags.ResetTo(U32Value);

	if (!Reader.Read(Out.DepthBias)) FAIL;
	if (!Reader.Read(Out.DepthBiasClamp)) FAIL;
	if (!Reader.Read(Out.SlopeScaledDepthBias)) FAIL;

	if (Out.Flags.Is(Render::CRenderStateDesc::DS_DepthEnable))
	{
		if (!Reader.Read<U8>(U8Value)) FAIL;
		Out.DepthFunc = (Render::ECmpFunc)U8Value;
	}

	if (Out.Flags.Is(Render::CRenderStateDesc::DS_StencilEnable))
	{
		if (!Reader.Read(Out.StencilReadMask)) FAIL;
		if (!Reader.Read(Out.StencilWriteMask)) FAIL;
		if (!Reader.Read<U32>(U32Value)) FAIL;
		Out.StencilRef = U32Value;

		if (!Reader.Read<U8>(U8Value)) FAIL;
		Out.StencilFrontFace.StencilFailOp = (Render::EStencilOp)U8Value;
		if (!Reader.Read<U8>(U8Value)) FAIL;
		Out.StencilFrontFace.StencilDepthFailOp = (Render::EStencilOp)U8Value;
		if (!Reader.Read<U8>(U8Value)) FAIL;
		Out.StencilFrontFace.StencilPassOp = (Render::EStencilOp)U8Value;
		if (!Reader.Read<U8>(U8Value)) FAIL;
		Out.StencilFrontFace.StencilFunc = (Render::ECmpFunc)U8Value;

		if (!Reader.Read<U8>(U8Value)) FAIL;
		Out.StencilBackFace.StencilFailOp = (Render::EStencilOp)U8Value;
		if (!Reader.Read<U8>(U8Value)) FAIL;
		Out.StencilBackFace.StencilDepthFailOp = (Render::EStencilOp)U8Value;
		if (!Reader.Read<U8>(U8Value)) FAIL;
		Out.StencilBackFace.StencilPassOp = (Render::EStencilOp)U8Value;
		if (!Reader.Read<U8>(U8Value)) FAIL;
		Out.StencilBackFace.StencilFunc = (Render::ECmpFunc)U8Value;
	}

	for (UPTR BlendIdx = 0; BlendIdx < 8; ++BlendIdx)
	{
		if (BlendIdx > 0 && Out.Flags.IsNot(Render::CRenderStateDesc::Blend_Independent)) break;

		Render::CRenderStateDesc::CRTBlend& RTBlend = Out.RTBlend[BlendIdx];

		if (!Reader.Read(RTBlend.WriteMask)) FAIL;

		if (Out.Flags.IsNot(Render::CRenderStateDesc::Blend_RTBlendEnable << BlendIdx)) continue;

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

	if (!Reader.Read(Out.BlendFactorRGBA[0])) FAIL;
	if (!Reader.Read(Out.BlendFactorRGBA[1])) FAIL;
	if (!Reader.Read(Out.BlendFactorRGBA[2])) FAIL;
	if (!Reader.Read(Out.BlendFactorRGBA[3])) FAIL;
	if (!Reader.Read<U32>(U32Value)) FAIL;
	Out.SampleMask = U32Value;

	if (!Reader.Read(Out.AlphaTestRef)) FAIL;
	if (!Reader.Read<U8>(U8Value)) FAIL;
	Out.AlphaTestFunc = (Render::ECmpFunc)U8Value;

	if (!Reader.Read(Out.VertexShader)) FAIL;
	if (!Reader.Read(Out.PixelShader)) FAIL;
	if (!Reader.Read(Out.GeometryShader)) FAIL;
	if (!Reader.Read(Out.HullShader)) FAIL;
	if (!Reader.Read(Out.DomainShader)) FAIL;
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
	if (!Reader.Read<U32>(Magic) || Magic != 'SHFX') return nullptr;

	U32 Version;
	if (!Reader.Read<U32>(Version) || Version > 0x00010000) return nullptr;

	CStrID MaterialType;
	if (!Reader.Read(MaterialType)) return nullptr;

	//???FIXME: use loaded CStrID as is?
	Render::EEffectType EffectType;
	if (MaterialType == "Opaque")
		EffectType = Render::EffectType_Opaque;
	else if (MaterialType == "AlphaTest")
		EffectType = Render::EffectType_AlphaTest;
	else if (MaterialType == "Skybox")
		EffectType = Render::EffectType_Skybox;
	else if (MaterialType == "AlphaBlend")
		EffectType = Render::EffectType_AlphaBlend;
	else
		EffectType = Render::EffectType_Other;

	U32 ShaderFormatCount;
	if (!Reader.Read<U32>(ShaderFormatCount) || !ShaderFormatCount) return nullptr;

	std::map<U32, U32> Offsets;
	for (U32 i = 0; i < ShaderFormatCount; ++i)
	{
		U32 Format;
		if (!Reader.Read(Format)) return nullptr;
		U32 Offset;
		if (!Reader.Read(Offset)) return nullptr;

		if (pGPU->SupportsShaderFormat(Format))
			Offsets.emplace(Format, Offset);
	}

	for (const auto& Pair : Offsets)
	{
		Reader.GetStream().Seek(Pair.second, IO::Seek_Begin);

		// Load param tables

		// Skip global metadata, actual is loaded in a render path
		auto GlobalMetaSize = Reader.Read<U32>();
		Reader.GetStream().Seek(GlobalMetaSize, IO::Seek_Current);

		Render::PShaderParamTable MaterialParams = pGPU->LoadShaderParamTable(Pair.first, Reader.GetStream());
		if (!MaterialParams) return nullptr;

		// Load material default values

		Render::CShaderParamValues MaterialDefaults;
		if (!LoadShaderParamValues(Reader, MaterialDefaults)) FAIL;

		// Load techniques and select the best one for each input set

		struct CTechniqueData
		{
			std::vector<U32> RSIndices;
			Render::PShaderParamTable Params;
		};

		std::set<U32> UsedRenderStateIndices;
		std::map<CStrID, CTechniqueData> TechPerInputSet;

		U32 TechCount;
		if (!Reader.Read(TechCount)) return nullptr;
		for (U32 TechIdx = 0; TechIdx < TechCount; ++TechIdx)
		{
			U32 FeatureLevel;
			if (!Reader.Read(FeatureLevel)) return nullptr;

			U32 SkipOffset;
			if (!Reader.Read(SkipOffset)) return nullptr;

			if (static_cast<Render::EGPUFeatureLevel>(FeatureLevel) > pGPU->GetFeatureLevel())
			{
				Reader.GetStream().Seek(SkipOffset, IO::Seek_Begin);
				continue;
			}

			CStrID InputSet;
			if (!Reader.Read(InputSet)) return nullptr;

			if (TechPerInputSet.find(InputSet) != TechPerInputSet.cend())
			{
				// Better valid tech is already found, skip the rest of techs for this input set
				// NB: we trust our tools that this tech is compatible with our GPU. This is checked
				// with FeatureLevel. So if some render states will happen to be invalid, it is
				// considered a fatal error and effect will not be loaded at all.
				Reader.GetStream().Seek(SkipOffset, IO::Seek_Begin);
				continue;
			}

			U32 PassCount;
			if (!Reader.Read(PassCount)) return nullptr;

			CTechniqueData TechData;
			TechData.RSIndices.resize(PassCount);
			for (auto& RSIndex : TechData.RSIndices)
			{
				if (!Reader.Read(RSIndex)) return nullptr;
			}

			TechData.Params = pGPU->LoadShaderParamTable(Pair.first, Reader.GetStream());
			if (!TechData.Params) return nullptr;

			// Tech is valid, mark its render states as used
			for (auto RSIndex : TechData.RSIndices)
				UsedRenderStateIndices.insert(RSIndex);
		}

		// Load only used render states

		U32 RenderStateCount;
		if (!Reader.Read(RenderStateCount)) return nullptr;

		std::vector<Render::PRenderState> RenderStates(RenderStateCount);
		for (U32 RSIndex = 0; RSIndex < RenderStateCount; ++RSIndex)
		{
			// TODO: can add skipping unused RS desc (even with precomputed offset) if optimization required
			Render::CRenderStateDesc Desc;
			if (!LoadRenderStateDesc(Reader, Desc)) return nullptr;

			// Load only used render states
			if (UsedRenderStateIndices.find(RSIndex) != UsedRenderStateIndices.cend())
			{
				RenderStates[RSIndex] = pGPU->CreateRenderState(Desc);
				if (!RenderStates[RSIndex]) return nullptr;
			}
		}

		// Create an effect from the loaded data

		Render::PEffect Effect = n_new(Render::CEffect(EffectType, MaterialParams, std::move(MaterialDefaults)));

		for (const auto& Pair : TechPerInputSet)
		{
			std::vector<Render::PRenderState> Passes;
			for (auto RSIndex : Pair.second.RSIndices)
				Passes.push_back(RenderStates[RSIndex]);

			Render::PTechnique Tech = n_new(Render::CTechnique(CStrID::Empty, std::move(Passes), -1, Pair.second.Params));
			Effect->SetTechnique(Pair.first, Tech);
		}

		return Effect;
	}

	// There is no effect variation for supported shader format
	return nullptr;
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

	Render::PEffect Effect = GetEffect(EffectID);

	// Build parameters

	Render::CShaderParamValues Values;
	if (!LoadShaderParamValues(Reader, Values)) FAIL;

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

}
