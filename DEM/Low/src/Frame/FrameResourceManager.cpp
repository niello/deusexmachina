#include "FrameResourceManager.h"
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
#include <Data/RAMData.h>
#include <Data/StringUtils.h>

namespace Frame
{

Render::PMesh CFrameResourceManager::GetMesh(CStrID UID)
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

	Meshes.emplace(UID, std::move(Mesh));

	return Mesh;
}
//---------------------------------------------------------------------

Render::PTexture CFrameResourceManager::GetTexture(CStrID UID, UPTR AccessFlags)
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

	if (Texture) Textures.emplace(UID, std::move(Texture));

	return Texture;
}
//---------------------------------------------------------------------

Render::PShader CFrameResourceManager::GetShader(CStrID UID)
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

	if (Shader) Shaders.emplace(UID, std::move(Shader));

	return Shader;
}
//---------------------------------------------------------------------

Render::PEffect CFrameResourceManager::GetEffect(CStrID UID)
{
	auto It = Effects.find(UID);
	if (It != Effects.cend() && It->second) return It->second;

	if (!pResMgr || !pGPU) return nullptr;

	const char* pSubId;
	IO::PStream Stream = pResMgr->CreateResourceStream(UID.CStr(), pSubId);

	if (!Stream || !Stream->Open(IO::SAM_READ, IO::SAP_SEQUENTIAL) || !Stream->CanRead()) return nullptr;

	Render::PEffect Effect = n_new(Render::CEffect);
	if (!Effect->Load(*this, *Stream)) return nullptr;

	Effects.emplace(UID, std::move(Effect));

	return Effect;
}
//---------------------------------------------------------------------

Render::PMaterial CFrameResourceManager::GetMaterial(CStrID UID)
{
	auto It = Materials.find(UID);
	if (It != Materials.cend() && It->second) return It->second;

	if (!pResMgr || !pGPU) return nullptr;

	const char* pSubId;
	IO::PStream Stream = pResMgr->CreateResourceStream(UID.CStr(), pSubId);

	if (!Stream || !Stream->Open(IO::SAM_READ, IO::SAP_SEQUENTIAL) || !Stream->CanRead()) return nullptr;

	Render::PMaterial Material = n_new(Render::CMaterial);
	if (!Material->Load(*this, *Stream)) return nullptr;

	Materials.emplace(UID, std::move(Material));

	return Material;
}
//---------------------------------------------------------------------

}
