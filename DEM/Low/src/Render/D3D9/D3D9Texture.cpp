#include "D3D9Texture.h"

#include <Render/D3D9/D3D9DriverFactory.h>
#include <Render/TextureData.h>
#include <Render/ImageUtils.h>
#include <Core/Factory.h>
#include "DEMD3D9.h"
#if DEM_RENDER_DEBUG
#include <D3Dcommon.h> // WKPDID_D3DDebugObjectName
#endif

namespace Render
{
FACTORY_CLASS_IMPL(Render::CD3D9Texture, 'TEX9', Render::CTexture);

bool CD3D9Texture::Create(PTextureData Data, UINT Usage, D3DPOOL Pool, IDirect3DBaseTexture9* pTexture, bool HoldRAMCopy)
{
	if (!pTexture || !Data) FAIL;

	TextureData = Data;
	pD3DTex = pTexture;
	D3DUsage = Usage;
	D3DPool = Pool;

	Access.ClearAll();
	if (Pool == D3DPOOL_SYSTEMMEM || Pool == D3DPOOL_SCRATCH)
	{
		Access.Set(Access_CPU_Read | Access_CPU_Write);
	}
	else
	{
		Access.Set(Access_GPU_Read);
		if (Usage & D3DUSAGE_DYNAMIC) Access.Set(Access_CPU_Write);
		else Access.Set(Access_GPU_Write);
	}

	HoldRAMBackingData = HoldRAMCopy;
	if (HoldRAMCopy) n_verify(TextureData->UseBuffer());

	switch (Data->Desc.Type)
	{
		case Texture_1D:
		case Texture_2D:
		case Texture_Cube:
		{
			D3DFORMAT Fmt = CD3D9DriverFactory::PixelFormatToD3DFormat(TextureData->Desc.Format);
			const bool IsBlockCompressed = (CD3D9DriverFactory::D3DFormatBlockSize(Fmt) > 1);
			RowPitch = CalcImageRowPitch(CD3D9DriverFactory::D3DFormatBitsPerPixel(Fmt), TextureData->Desc.Width, IsBlockCompressed);
			SlicePitch = 0;
			break;
		}
		case Texture_3D:
		{
			D3DFORMAT Fmt = CD3D9DriverFactory::PixelFormatToD3DFormat(TextureData->Desc.Format);
			const bool IsBlockCompressed = (CD3D9DriverFactory::D3DFormatBlockSize(Fmt) > 1);
			RowPitch = CalcImageRowPitch(CD3D9DriverFactory::D3DFormatBitsPerPixel(Fmt), TextureData->Desc.Width, IsBlockCompressed);
			SlicePitch = CalcImageSlicePitch(RowPitch, TextureData->Desc.Height, IsBlockCompressed);
			break;
		}
	}

	OK;
}
//---------------------------------------------------------------------

void CD3D9Texture::InternalDestroy()
{
//	n_assert(!LockCount);
	SAFE_RELEASE(pD3DTex);
}
//---------------------------------------------------------------------

IDirect3DTexture9* CD3D9Texture::GetD3DTexture() const
{
	n_assert(/*!LockCount &&*/ TextureData->Desc.Type == Texture_2D);
	return static_cast<IDirect3DTexture9*>(pD3DTex);
}
//---------------------------------------------------------------------

IDirect3DCubeTexture9* CD3D9Texture::GetD3DCubeTexture() const
{
	n_assert(/*!LockCount &&*/ TextureData->Desc.Type == Texture_Cube);
	return static_cast<IDirect3DCubeTexture9*>(pD3DTex);
}
//---------------------------------------------------------------------

IDirect3DVolumeTexture9* CD3D9Texture::GetD3DVolumeTexture() const
{
	n_assert(/*!LockCount &&*/ TextureData->Desc.Type == Texture_3D);
	return static_cast<IDirect3DVolumeTexture9*>(pD3DTex);
}
//---------------------------------------------------------------------

void CD3D9Texture::SetDebugName(std::string_view Name)
{
#if DEM_RENDER_DEBUG
	if (pD3DTex) pD3DTex->SetPrivateData(WKPDID_D3DDebugObjectName, Name.data(), Name.size(), 0);
#endif
}
//---------------------------------------------------------------------

}
