#include "D3D11DepthStencilBuffer.h"

#include <Core/Factory.h>
#define WIN32_LEAN_AND_MEAN
#include <d3d11.h>

namespace Render
{
__ImplementClass(Render::CD3D11DepthStencilBuffer, 'DSB1', Render::CDepthStencilBuffer);

//!!!???assert destroyed?!
bool CD3D11DepthStencilBuffer::Create(ID3D11DepthStencilView* pDSV)
{
	n_assert(pDSV);

	//!!!fill desc!

	pDSView = pDSV;
	OK;
}
//---------------------------------------------------------------------

void CD3D11DepthStencilBuffer::Destroy()
{
	SAFE_RELEASE(pDSView);
}
//---------------------------------------------------------------------

}