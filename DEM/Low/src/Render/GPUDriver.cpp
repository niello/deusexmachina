#include "GPUDriver.h"

#include <Render/VertexLayout.h>
#include <Render/Texture.h>
#include <Render/TextureData.h>
#include <System/OSWindow.h>
#include <Resources/ResourceManager.h>
#include <Resources/Resource.h>
#include <Data/Params.h>

namespace Render
{

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

//???validate existing texture here? it may be lost (at least in D3D9). Some other invalid state?
//???bool keep loaded into RAM? how to control multiple clients? some kind of refcounting?
PTexture CGPUDriver::GetTexture(CStrID UID, UPTR AccessFlags)
{
	PTexture Texture;
	if (ResourceTextures.Get(UID, Texture) && Texture)
	{
		n_assert(Texture->GetAccess().Is(AccessFlags));
		return Texture;
	}

	if (!pResMgr) return nullptr;

	Resources::PResource RTexData = pResMgr->RegisterResource<CTextureData>(UID);
	PTextureData TexData = RTexData->ValidateObject<CTextureData>();
	Texture = CreateTexture(TexData->Desc, AccessFlags, TexData->pData, TexData->MipDataProvided);

	//!!!not to keep in RAM! or use refcount
	//???CTexture holds resource ref if it wants RAM backing?
	//TexData->UnloadResource();

	if (Texture) ResourceTextures.Add(UID, Texture);

	return Texture;
}
//---------------------------------------------------------------------

}