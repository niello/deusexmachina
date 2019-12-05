#include "D3D11RenderState.h"
#include <Render/D3D11/D3D11Shader.h>
#define WIN32_LEAN_AND_MEAN
#include <d3d11.h>

namespace Render
{
RTTI_CLASS_IMPL(Render::CD3D11RenderState, Render::CRenderState);

CD3D11RenderState::CD3D11RenderState() = default;

CD3D11RenderState::~CD3D11RenderState()
{
	SAFE_RELEASE(pRState);
	SAFE_RELEASE(pDSState);
	SAFE_RELEASE(pBState);
}
//---------------------------------------------------------------------

}