#include "UIRenderer.h"

#include <CEGUISystem.h>

namespace Render
{
ImplementRTTI(Render::CUIRenderer, Render::IRenderer);
ImplementFactory(Render::CUIRenderer);

void CUIRenderer::Render()
{
	CEGUI::System::getSingleton().renderGUI();
}
//---------------------------------------------------------------------

}