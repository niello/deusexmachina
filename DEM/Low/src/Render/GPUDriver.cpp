#include "GPUDriver.h"

#include <Render/VertexLayout.h>
#include <Render/Texture.h>
#include <Render/TextureData.h>
#include <Render/Shader.h>
#include <Render/ShaderLibrary.h>
#include <Render/Effect.h>
#include <Render/Material.h>
#include <Render/Mesh.h>
#include <Render/MeshData.h>
#include <System/OSWindow.h>
#include <Resources/ResourceManager.h>
#include <Resources/Resource.h>
#include <IO/Stream.h>
#include <Data/Params.h>
#include <Data/RAMData.h>
#include <Data/StringUtils.h>

namespace Render
{
CGPUDriver::CGPUDriver() {}
CGPUDriver::~CGPUDriver() {}

bool CGPUDriver::PresentBlankScreen(UPTR SwapChainID, const vector4& ColorRGBA)
{
	if (!BeginFrame()) FAIL;
	Clear(Clear_Color, ColorRGBA, 1.f, 0);
	EndFrame();
	return Present(SwapChainID);
}
//---------------------------------------------------------------------

void CGPUDriver::PrepareWindowAndBackBufferSize(DEM::Sys::COSWindow& Window, U32& Width, U32& Height)
{
	// Zero Width or Height means backbuffer matching window or display size.
	// But if at least one of these values specified, we should adjst window size.
	// A child window is an exception, we don't want renderer to resize it,
	// so we force a backbuffer size to a child window size.
	if (Width > 0 || Height > 0)
	{
		if (Window.IsChild())
		{
			Width = Window.GetWidth();
			Height = Window.GetHeight();
		}
		else
		{
			Data::CRect WindowRect = Window.GetRect();
			if (Width > 0) WindowRect.W = Width;
			else Width = WindowRect.W;
			if (Height > 0) WindowRect.H = Height;
			else Height = WindowRect.H;
			Window.SetRect(WindowRect);
		}
	}
}
//---------------------------------------------------------------------

void CGPUDriver::SetResourceManager(Resources::CResourceManager* pResourceManager)
{
	if (pResMgr == pResourceManager) return;

	//???destroy resource links? if Rs are stored instead of UIDs
	pResMgr = pResourceManager;
}
//---------------------------------------------------------------------

PTexture CGPUDriver::GetTexture(CStrID UID, UPTR AccessFlags)
{
	PTexture Texture;
	if (Textures.Get(UID, Texture) && Texture)
	{
		n_assert(Texture->GetAccess().Is(AccessFlags));
		return Texture;
	}

	if (!pResMgr) return nullptr;

	Resources::PResource RTexData = pResMgr->RegisterResource<CTextureData>(UID);
	PTextureData TexData = RTexData->ValidateObject<CTextureData>();

	if (!TexData->UseRAMData()) return nullptr;

	Texture = CreateTexture(TexData, AccessFlags);

	TexData->ReleaseRAMData();

	if (Texture) Textures.Add(UID, Texture);

	return Texture;
}
//---------------------------------------------------------------------

PShader CGPUDriver::GetShader(CStrID UID)
{
	PShader Shader;
	if (Shaders.Get(UID, Shader) && Shader) return Shader;

	// Tough resource manager doesn't keep track of shaders, it is used
	// for shader library loading and for accessing an IO service.
	if (!pResMgr) return nullptr;

	IO::PStream Stream;
	PShaderLibrary ShaderLibrary;
	const char* pSubId = strchr(UID.CStr(), '#');
	if (pSubId)
	{
		// Generated shaders are not supported, must be a file
		if (pSubId == UID.CStr()) return nullptr;

		// File must be a ShaderLibrary if sub-ID is used
		CString Path(UID.CStr(), pSubId - UID.CStr());
		++pSubId; // Skip '#'
		if (*pSubId == 0) return nullptr;

		Resources::PResource RShaderLibrary = pResMgr->RegisterResource<CShaderLibrary>(CStrID(Path));
		if (!RShaderLibrary) return nullptr;

		ShaderLibrary = RShaderLibrary->ValidateObject<CShaderLibrary>();
		if (!ShaderLibrary) return nullptr;

		const U32 ElementID = StringUtils::ToInt(pSubId);
		Stream = ShaderLibrary->GetElementStream(ElementID);
	}
	else Stream = pResMgr->CreateResourceStream(UID, pSubId);

	if (!Stream || !Stream->Open(IO::SAM_READ, IO::SAP_SEQUENTIAL) || !Stream->CanRead()) return nullptr;

	Shader = CreateShader(*Stream, ShaderLibrary.Get());

	if (Shader) Shaders.Add(UID, Shader);

	return Shader;
}
//---------------------------------------------------------------------

PEffect CGPUDriver::GetEffect(CStrID UID)
{
	PEffect Effect;
	if (Effects.Get(UID, Effect) && Effect) return Effect;

	if (!pResMgr) return nullptr;

	const char* pSubId;
	IO::PStream Stream = pResMgr->CreateResourceStream(UID.CStr(), pSubId);

	if (!Stream || !Stream->Open(IO::SAM_READ, IO::SAP_SEQUENTIAL) || !Stream->CanRead()) return nullptr;

	Effect = n_new(CEffect);
	if (!Effect->Load(*this, *Stream)) return nullptr;

	Effects.Add(UID, Effect);

	return Effect;
}
//---------------------------------------------------------------------

PMaterial CGPUDriver::GetMaterial(CStrID UID)
{
	PMaterial Material;
	if (Materials.Get(UID, Material) && Material) return Material;

	if (!pResMgr) return nullptr;

	const char* pSubId;
	IO::PStream Stream = pResMgr->CreateResourceStream(UID.CStr(), pSubId);

	if (!Stream || !Stream->Open(IO::SAM_READ, IO::SAP_SEQUENTIAL) || !Stream->CanRead()) return nullptr;

	Material = n_new(CMaterial);
	if (!Material->Load(*this, *Stream)) return nullptr;

	Materials.Add(UID, Material);

	return Material;
}
//---------------------------------------------------------------------

PMesh CGPUDriver::GetMesh(CStrID UID)
{
	PMesh Mesh;
	if (Meshes.Get(UID, Mesh) && Mesh) return Mesh;

	if (!pResMgr) return nullptr;

	Resources::PResource RMeshData = pResMgr->RegisterResource<CMeshData>(UID);
	PMeshData MeshData = RMeshData->ValidateObject<CMeshData>();

	if (!MeshData->UseRAMData()) return nullptr;

	//!!!Now all VBs and IBs are not shared! later this may change!

	PVertexLayout VertexLayout = CreateVertexLayout(&MeshData->VertexFormat.Front(), MeshData->VertexFormat.GetCount());
	PVertexBuffer VB = CreateVertexBuffer(*VertexLayout, MeshData->VertexCount, Render::Access_GPU_Read, MeshData->VBData->GetPtr());
	PIndexBuffer IB;
	if (MeshData->IndexCount)
		IB = CreateIndexBuffer(MeshData->IndexType, MeshData->IndexCount, Render::Access_GPU_Read, MeshData->IBData->GetPtr());

	Mesh = n_new(Render::CMesh);
	bool Result = Mesh->Create(MeshData, VB, IB);

	MeshData->ReleaseRAMData();

	if (!Result) return nullptr;

	Meshes.Add(UID, Mesh);

	return Mesh;
}
//---------------------------------------------------------------------

}