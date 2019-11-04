#include "GraphicsResourceManager.h"
#include <Frame/RenderPath.h>
#include <Frame/RenderPhase.h>
#include <Render/GPUDriver.h>
#include <Render/VertexLayout.h>
#include <Render/Texture.h>
#include <Render/TextureData.h>
#include <Render/Sampler.h>
#include <Render/ConstantBuffer.h>
#include <Render/Shader.h>
#include <Render/ShaderLibrary.h>
#include <Render/RenderState.h>
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
#include <Core/Factory.h>
#include <map>

namespace Frame
{

CGraphicsResourceManager::CGraphicsResourceManager(Resources::CResourceManager& ResMgr, Render::CGPUDriver& GPU)
	: pResMgr(&ResMgr)
	, GPU(&GPU)
{
}
//---------------------------------------------------------------------

CGraphicsResourceManager::~CGraphicsResourceManager() = default;
//---------------------------------------------------------------------

Render::CGPUDriver* CGraphicsResourceManager::GetGPU() const
{
	return GPU;
}
//---------------------------------------------------------------------

Render::PMesh CGraphicsResourceManager::GetMesh(CStrID UID)
{
	if (!UID) return nullptr;

	auto It = Meshes.find(UID);
	if (It != Meshes.cend() && It->second) return It->second;

	if (!pResMgr || !GPU) return nullptr;

	Resources::PResource RMeshData = pResMgr->RegisterResource<Render::CMeshData>(UID);
	Render::PMeshData MeshData = RMeshData->ValidateObject<Render::CMeshData>();

	if (!MeshData->UseRAMData()) return nullptr;

	//!!!Now all VBs and IBs are not shared! later this may change!

	Render::PVertexLayout VertexLayout = GPU->CreateVertexLayout(&MeshData->VertexFormat.Front(), MeshData->VertexFormat.GetCount());
	Render::PVertexBuffer VB = GPU->CreateVertexBuffer(*VertexLayout, MeshData->VertexCount, Render::Access_GPU_Read, MeshData->VBData->GetPtr());
	Render::PIndexBuffer IB;
	if (MeshData->IndexCount)
		IB = GPU->CreateIndexBuffer(MeshData->IndexType, MeshData->IndexCount, Render::Access_GPU_Read, MeshData->IBData->GetPtr());

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
	if (!UID) return nullptr;

	auto It = Textures.find(UID);
	if (It != Textures.cend() && It->second)
	{
		n_assert(It->second->GetAccess().Is(AccessFlags));
		return It->second;
	}

	if (!pResMgr || !GPU) return nullptr;

	Resources::PResource RTexData = pResMgr->RegisterResource<Render::CTextureData>(UID);
	Render::PTextureData TexData = RTexData->ValidateObject<Render::CTextureData>();

	if (!TexData->UseRAMData()) return nullptr;

	Render::PTexture Texture = GPU->CreateTexture(TexData, AccessFlags);

	TexData->ReleaseRAMData();

	if (Texture) Textures.emplace(UID, Texture);

	return Texture;
}
//---------------------------------------------------------------------

Render::PShader CGraphicsResourceManager::GetShader(CStrID UID, bool NeedParamTable)
{
	if (!UID) return nullptr;

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
	if (!pResMgr || !GPU) return nullptr;

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

	Render::PShader Shader = GPU->CreateShader(*Stream, ShaderLibrary.Get(), NeedParamTable);

	if (Shader) Shaders.emplace(UID, Shader);

	return Shader;
}
//---------------------------------------------------------------------

Render::PEffect CGraphicsResourceManager::GetEffect(CStrID UID)
{
	if (!UID) return nullptr;

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
	if (!UID) return nullptr;

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
	if (!UID) return nullptr;

	auto It = RenderPaths.find(UID);
	if (It != RenderPaths.cend() && It->second) return It->second;

	PRenderPath RP = LoadRenderPath(UID);
	if (!RP) return nullptr;

	RenderPaths.emplace(UID, RP);

	return RP;
}
//---------------------------------------------------------------------

bool CGraphicsResourceManager::LoadShaderParamValues(IO::CBinaryReader& Reader, const Render::CShaderParamTable& MaterialTable, Render::CShaderParamValues& Out)
{
	Out.ConstValueBuffer.reset();
	Out.ConstValues.clear();
	Out.ResourceValues.clear();
	Out.SamplerValues.clear();

	U32 ParamCount;
	if (!Reader.Read<U32>(ParamCount)) FAIL;
	for (UPTR ParamIdx = 0; ParamIdx < ParamCount; ++ParamIdx)
	{
		CStrID ParamID;
		if (!Reader.Read(ParamID)) FAIL;

		if (MaterialTable.GetConstant(ParamID))
		{
			U32 Offset;
			if (!Reader.Read(Offset)) FAIL;
			U32 Size;
			if (!Reader.Read(Size)) FAIL;
			Out.ConstValues.emplace(ParamID, Render::CShaderConstValue{ (void*)Offset, Size });
		}
		else if (MaterialTable.GetResource(ParamID))
		{
			CStrID RUID;
			if (!Reader.Read(RUID)) FAIL;
			Render::PTexture Texture = GetTexture(RUID, Render::Access_GPU_Read);
			if (Texture.IsNullPtr()) FAIL;
			Out.ResourceValues.emplace(ParamID, Texture);
		}
		else if (MaterialTable.GetSampler(ParamID))
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

			Render::PSampler Sampler = GPU->CreateSampler(SamplerDesc);
			if (Sampler.IsNullPtr()) FAIL;
			Out.SamplerValues.emplace(ParamID, Sampler);
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
			Pair.second.pData = Out.ConstValueBuffer.get() + (U32)Pair.second.pData;
	}

	OK;
}
//---------------------------------------------------------------------

bool CGraphicsResourceManager::LoadRenderStateDesc(IO::CBinaryReader& Reader, Render::CRenderStateDesc& Out, bool LoadParamTables)
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

	CStrID VertexShaderID;
	CStrID PixelShaderID;
	CStrID GeometryShaderID;
	CStrID HullShaderID;
	CStrID DomainShaderID;
	if (!Reader.Read(VertexShaderID)) FAIL;
	if (!Reader.Read(PixelShaderID)) FAIL;
	if (!Reader.Read(GeometryShaderID)) FAIL;
	if (!Reader.Read(HullShaderID)) FAIL;
	if (!Reader.Read(DomainShaderID)) FAIL;

	Out.VertexShader = GetShader(VertexShaderID, LoadParamTables);
	Out.PixelShader = GetShader(PixelShaderID, LoadParamTables);
	Out.GeometryShader = GetShader(GeometryShaderID, LoadParamTables);
	Out.HullShader = GetShader(HullShaderID, LoadParamTables);
	Out.DomainShader = GetShader(DomainShaderID, LoadParamTables);

	OK;
}
//---------------------------------------------------------------------

Render::PEffect CGraphicsResourceManager::LoadEffect(CStrID UID)
{
	if (!pResMgr || !GPU) return nullptr;

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

		if (GPU->SupportsShaderFormat(Format))
			Offsets.emplace(Format, Offset);
	}

	U32 MaterialDefaultsOffset;
	if (!Reader.Read(MaterialDefaultsOffset)) return nullptr;

	std::map<CStrID, Render::PTechnique> Techs;
	Render::PShaderParamTable MaterialParams;

	for (const auto& Pair : Offsets)
	{
		MaterialParams.Reset();

		Reader.GetStream().Seek(Pair.second, IO::Seek_Begin);

		// Load param tables

		// Skip global metadata, actual is loaded in a render path
		auto GlobalMetaSize = Reader.Read<U32>();
		Reader.GetStream().Seek(GlobalMetaSize, IO::Seek_Current);

		// Skip material meta size, read table
		Reader.Read<U32>();
		MaterialParams = GPU->LoadShaderParamTable(Pair.first, Reader.GetStream());
		if (!MaterialParams) return nullptr;

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

			if (static_cast<Render::EGPUFeatureLevel>(FeatureLevel) > GPU->GetFeatureLevel())
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

			// Skip tech meta size, read table
			Reader.Read<U32>();
			TechData.Params = GPU->LoadShaderParamTable(Pair.first, Reader.GetStream());
			if (!TechData.Params) return nullptr;

			// Tech is valid, mark its render states as used
			for (auto RSIndex : TechData.RSIndices)
				UsedRenderStateIndices.insert(RSIndex);

			TechPerInputSet.emplace(InputSet, std::move(TechData));
		}

		// Load only used render states

		U32 RenderStateCount;
		if (!Reader.Read(RenderStateCount)) return nullptr;

		std::vector<Render::PRenderState> RenderStates(RenderStateCount);
		for (U32 RSIndex = 0; RSIndex < RenderStateCount; ++RSIndex)
		{
			// Load render state from file, skip shader param tables because effect already has them
			// TODO: can seek to the end of unused RS desc (with precomputed offset?) if optimization required
			Render::CRenderStateDesc Desc;
			if (!LoadRenderStateDesc(Reader, Desc, false)) return nullptr;

			// Load only used render states
			if (UsedRenderStateIndices.find(RSIndex) != UsedRenderStateIndices.cend())
			{
				RenderStates[RSIndex] = GPU->CreateRenderState(Desc);
				if (!RenderStates[RSIndex]) return nullptr;
			}
		}

		// Create techs for the valid effect

		for (const auto& Pair : TechPerInputSet)
		{
			std::vector<Render::PRenderState> Passes;
			for (auto RSIndex : Pair.second.RSIndices)
				Passes.push_back(RenderStates[RSIndex]);

			Techs.emplace(Pair.first, n_new(Render::CTechnique(CStrID::Empty, std::move(Passes), -1, Pair.second.Params)));
		}

		break;
	}

	// There is no effect variation for supported shader format
	if (Techs.empty()) return nullptr;

	// Load material default values

	Reader.GetStream().Seek(MaterialDefaultsOffset, IO::Seek_Begin);

	Render::CShaderParamValues MaterialDefaults;
	if (!LoadShaderParamValues(Reader, *MaterialParams, MaterialDefaults)) FAIL;

	// Create an effect from the loaded data

	Render::PEffect Effect = n_new(Render::CEffect(EffectType, MaterialParams, std::move(MaterialDefaults)));

	for (const auto& Pair : Techs)
		Effect->SetTechnique(Pair.first, Pair.second);

	return Effect;
}
//---------------------------------------------------------------------

Render::PMaterial CGraphicsResourceManager::LoadMaterial(CStrID UID)
{
	if (!pResMgr || !GPU) return nullptr;

	const char* pOutSubId;
	IO::PStream Stream = pResMgr->CreateResourceStream(UID, pOutSubId);
	if (!Stream || !Stream->Open(IO::SAM_READ, IO::SAP_SEQUENTIAL) || !Stream->CanRead()) return nullptr;

	IO::CBinaryReader Reader(*Stream);

	U32 Magic;
	if (!Reader.Read<U32>(Magic) || Magic != 'MTRL') return nullptr;

	U32 Version;
	if (!Reader.Read<U32>(Version)) return nullptr;

	CStrID EffectID;
	if (!Reader.Read(EffectID)) return nullptr;
	if (!EffectID.IsValid()) return nullptr;

	Render::PEffect Effect = GetEffect(EffectID);
	if (!Effect) return nullptr;

	// Build parameters

	auto& Table = Effect->GetMaterialParamTable();

	Render::CShaderParamValues Values;
	if (!LoadShaderParamValues(Reader, Table, Values)) return nullptr;

	Render::CShaderParamStorage Storage(Table, *GPU);

	// Constants will be filled into RAM buffers, and they will be used as initial data for immutable GPU buffers.
	// D3D9 buffer is CPU+GPU, so it can be reused directly, but D3D11 requires data transfer.
	// TODO: check what is faster, to use CD3D11ConstantBuffer with D3D11_USAGE_STAGING or create CD3D11RAMConstantBuffer.
	std::vector<Render::PConstantBuffer> RAMBuffers(Table.GetConstantBuffers().size());
	for (size_t i = 0; i < Table.GetConstantBuffers().size(); ++i)
	{
		RAMBuffers[i] = GPU->CreateConstantBuffer(*Table.GetConstantBuffer(i), Render::Access_CPU_Write);
		if (!GPU->BeginShaderConstants(*RAMBuffers[i])) return nullptr;
	}

	// Fill intermediate buffers with constant values
	for (size_t i = 0; i < Table.GetConstants().size(); ++i)
	{
		auto Param = Table.GetConstant(i);
		auto It = Values.ConstValues.find(Param.GetID());
		const Render::CShaderConstValue* pValue = (It != Values.ConstValues.cend()) ? &It->second : Effect->GetConstantDefaultValue(Param.GetID());
		if (pValue) Param.SetRawValue(*RAMBuffers[Param.GetConstantBufferIndex()], pValue->pData, pValue->Size);
	}

	// Set filled constant buffers
	for (size_t i = 0; i < Table.GetConstantBuffers().size(); ++i)
	{
		auto& Buffer = RAMBuffers[i];
		if (!GPU->CommitShaderConstants(*Buffer)) return nullptr;

		// Transfer temporary data to immutable VRAM if required
		if (!(Buffer->GetAccessFlags() & Render::Access_GPU_Read))
			Buffer = GPU->CreateConstantBuffer(*Table.GetConstantBuffer(i), Render::Access_GPU_Read, Buffer.Get());

		Storage.SetConstantBuffer(i, Buffer);
	}

	// Set resources
	for (size_t i = 0; i < Table.GetResources().size(); ++i)
	{
		const CStrID ID = Table.GetResource(i)->GetID();
		auto It = Values.ResourceValues.find(ID);
		Storage.SetResource(i, (It != Values.ResourceValues.cend()) ? It->second : Effect->GetResourceDefaultValue(ID));
	}

	// Set samplers
	for (size_t i = 0; i < Table.GetSamplers().size(); ++i)
	{
		const CStrID ID = Table.GetSampler(i)->GetID();
		auto It = Values.SamplerValues.find(ID);
		Storage.SetSampler(i, (It != Values.SamplerValues.cend()) ? It->second : Effect->GetSamplerDefaultValue(ID));
	}

	return n_new(Render::CMaterial(*Effect, std::move(Storage)));
}
//---------------------------------------------------------------------

PRenderPath CGraphicsResourceManager::LoadRenderPath(CStrID UID)
{
	if (!pResMgr || !GPU) return nullptr;

	const char* pOutSubId;
	IO::PStream Stream = pResMgr->CreateResourceStream(UID, pOutSubId);
	if (!Stream || !Stream->Open(IO::SAM_READ, IO::SAP_SEQUENTIAL) || !Stream->CanRead()) return nullptr;

	IO::CBinaryReader Reader(*Stream);

	U32 Magic;
	if (!Reader.Read<U32>(Magic) || Magic != 'RPTH') return nullptr;

	U32 Version;
	if (!Reader.Read<U32>(Version)) return nullptr;

	Frame::PRenderPath RP = n_new(Frame::CRenderPath);

	// Read render targets

	U32 RTCount;
	if (!Reader.Read(RTCount)) return nullptr;

	for (U32 i = 0; i < RTCount; ++i)
	{
		CStrID ID;
		if (!Reader.Read(ID)) return nullptr;

		vector4 ClearValue;
		if (!Reader.Read(ClearValue)) return nullptr;

		RP->AddRenderTargetSlot(ID, ClearValue);
	}

	// Read depth-stencil buffers

	U32 DSCount;
	if (!Reader.Read(DSCount)) return nullptr;

	for (U32 i = 0; i < DSCount; ++i)
	{
		CStrID ID;
		if (!Reader.Read(ID)) return nullptr;

		float DepthClearValue;
		U8 StencilClearValue;
		U8 ClearFlags;
		if (!Reader.Read(DepthClearValue)) return nullptr;
		if (!Reader.Read(StencilClearValue)) return nullptr;
		if (!Reader.Read(ClearFlags)) return nullptr;

		constexpr U8 Flag_ClearDepth = (1 << 0);
		constexpr U8 Flag_ClearStencil = (1 << 1);

		U32 RealClearFlags = 0;
		if (ClearFlags & Flag_ClearDepth)
			RealClearFlags |= Render::Clear_Depth;
		if (ClearFlags & Flag_ClearStencil)
			RealClearFlags |= Render::Clear_Stencil;

		RP->AddDepthStencilSlot(ID, RealClearFlags, DepthClearValue, StencilClearValue);
	}

	// Read phases

	Data::PParams PhaseDescs = n_new(Data::CParams);
	if (!Reader.ReadParams(*PhaseDescs)) return nullptr;

	// Read global params compatible with our GPU

	U32 ShaderFormatCount;
	if (!Reader.Read<U32>(ShaderFormatCount)) return nullptr;

	std::map<U32, U32> Offsets;
	for (U32 i = 0; i < ShaderFormatCount; ++i)
	{
		U32 Format;
		if (!Reader.Read(Format)) return nullptr;
		U32 Offset;
		if (!Reader.Read(Offset)) return nullptr;

		if (GPU->SupportsShaderFormat(Format))
		{
			Reader.GetStream().Seek(Offset, IO::Seek_Begin);

			RP->Globals = GPU->LoadShaderParamTable(Format, Reader.GetStream());
			if (!RP->Globals) return nullptr;

			// NB: file has no useful data after this, so leave the stream at any position it is now
			break;
		}
	}

	// Create phases.
	// NB: intentionally created at the end, because they may access global params etc.

	std::vector<PRenderPhase> Phases(PhaseDescs->GetCount());
	for (UPTR i = 0; i < PhaseDescs->GetCount(); ++i)
	{
		const Data::CParam& Prm = PhaseDescs->Get(i);
		Data::CParams& PhaseDesc = *Prm.GetValue<Data::PParams>();

		const CString& PhaseType = PhaseDesc.Get<CString>(CStrID("Type"), CString::Empty); //???use FourCC in PRM?
		CString ClassName = "Frame::CRenderPhase" + PhaseType;
		Frame::PRenderPhase CurrPhase = (Frame::CRenderPhase*)Factory->Create(ClassName.CStr());

		if (!CurrPhase->Init(*RP.Get(), Prm.GetName(), PhaseDesc)) return nullptr;

		Phases[i] = CurrPhase;
	}

	RP->Phases = std::move(Phases);

	return RP.Get();
}
//---------------------------------------------------------------------

}
