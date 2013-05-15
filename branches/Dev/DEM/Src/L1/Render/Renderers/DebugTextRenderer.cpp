#include "DebugTextRenderer.h"

#include <Render/DebugDraw.h>

namespace Render
{
__ImplementClass(Render::CDebugTextRenderer, 'DBTR', Render::IRenderer);

void CDebugTextRenderer::Render()
{
	//???move rendering right here?
	if (DebugDraw->HasInstance()) DebugDraw->RenderText();
}
//---------------------------------------------------------------------

}