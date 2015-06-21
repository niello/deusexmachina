#include "D3D9DepthStencilBuffer.h"

#include <Core/Factory.h>
#define WIN32_LEAN_AND_MEAN
#define D3D_DISABLE_9EX
#include <d3d9.h>

namespace Render
{
__ImplementClass(Render::CD3D9DepthStencilBuffer, 'DSB9', Render::CDepthStencilBuffer);

//!!!???assert destroyed?!
bool CD3D9DepthStencilBuffer::Create(IDirect3DSurface9* pSurface)
{
	n_assert(pSurface);
	pDSSurface = pSurface;
	OK;
}
//---------------------------------------------------------------------

void CD3D9DepthStencilBuffer::Destroy()
{
	SAFE_RELEASE(pDSSurface);
}
//---------------------------------------------------------------------

}