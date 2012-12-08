#include "Renderer.h"

namespace Render
{
ImplementRTTI(Render::CRenderer, Core::CRefCounted);
__ImplementSingleton(CRenderer);

bool CRenderer::Open()
{
	n_assert(!_IsOpen);
	_IsOpen = true;
	OK;
}
//---------------------------------------------------------------------

void CRenderer::Close()
{
	n_assert(_IsOpen);
	_IsOpen = false;
}
//---------------------------------------------------------------------

}