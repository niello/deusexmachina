#include "DebugGeomRenderer.h"

#include <Render/DebugDraw.h>

namespace Render
{
__ImplementClass(Render::CDebugGeomRenderer, 'DBGM', Render::IRenderer);

void CDebugGeomRenderer::Render()
{
	//???move rendering right here?
	if (DebugDraw->HasInstance()) DebugDraw->RenderGeometry();
}
//---------------------------------------------------------------------

}