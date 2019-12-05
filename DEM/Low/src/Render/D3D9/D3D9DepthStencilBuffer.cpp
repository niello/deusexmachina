#include "D3D9DepthStencilBuffer.h"

#include <Render/D3D9/D3D9DriverFactory.h>
#include <Core/Factory.h>
#include "DEMD3D9.h"

namespace Render
{
FACTORY_CLASS_IMPL(Render::CD3D9DepthStencilBuffer, 'DSB9', Render::CDepthStencilBuffer);

//!!!???assert destroyed?!
bool CD3D9DepthStencilBuffer::Create(IDirect3DSurface9* pSurface)
{
	n_assert(pSurface);

	D3DSURFACE_DESC DSDesc;
	if (FAILED(pSurface->GetDesc(&DSDesc)) || !(DSDesc.Usage & D3DUSAGE_DEPTHSTENCIL)) FAIL;

	Desc.Width = DSDesc.Width;
	Desc.Height = DSDesc.Height;
	Desc.Format = CD3D9DriverFactory::D3DFormatToPixelFormat(DSDesc.Format);
	Desc.MSAAQuality = CD3D9DriverFactory::D3DMSAAParamsToMSAAQuality(DSDesc.MultiSampleType, DSDesc.MultiSampleQuality);
	Desc.UseAsShaderInput = false; //!!!may add support!

	//!!!validate format if shader input!
	//if (Texture.IsValidPtr())
	//{
	//	IDirect3DSurface9* pTmpSurf = nullptr;
	//	if (FAILED(Texture->GetD3DTexture()->GetSurfaceLevel(0, &pTmpSurf))) FAIL;
	//	NeedResolve = (pTmpSurf != pSurface);
	//	Desc.UseAsShaderInput = true;
	//	SRTexture = Texture;
	//}
	//else
	//{
	//	NeedResolve = false;
	//	Desc.UseAsShaderInput = false;
	//	SRTexture = nullptr;
	//}

	pDSSurface = pSurface;
	OK;
}
//---------------------------------------------------------------------

void CD3D9DepthStencilBuffer::InternalDestroy()
{
	SAFE_RELEASE(pDSSurface);
}
//---------------------------------------------------------------------

}