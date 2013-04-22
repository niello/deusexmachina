#include "DebugGeomRenderer.h"

#include <Render/DebugDraw.h>

namespace Render
{
ImplementRTTI(Render::CDebugGeomRenderer, Render::IRenderer);
ImplementFactory(Render::CDebugGeomRenderer);

void CDebugGeomRenderer::Render()
{
	//???move rendering right here?
	if (DebugDraw->HasInstance()) DebugDraw->RenderGeometry();
}
//---------------------------------------------------------------------

}