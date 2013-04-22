#include "DebugTextRenderer.h"

#include <Render/DebugDraw.h>

namespace Render
{
ImplementRTTI(Render::CDebugTextRenderer, Render::IRenderer);
ImplementFactory(Render::CDebugTextRenderer);

void CDebugTextRenderer::Render()
{
	//???move rendering right here?
	if (DebugDraw->HasInstance()) DebugDraw->RenderText();
}
//---------------------------------------------------------------------

}