#include "D3D9RenderState.h"

#include <Core/Factory.h>
#define WIN32_LEAN_AND_MEAN
#define D3D_DISABLE_9EX
#include <d3d9.h>

namespace Render
{
__ImplementClass(Render::CD3D9RenderState, 'RS09', Render::CRenderState);

CD3D9RenderState::~CD3D9RenderState()
{
	SAFE_RELEASE(pVS);
	SAFE_RELEASE(pPS);
	SAFE_RELEASE(pState);
}
//---------------------------------------------------------------------

}