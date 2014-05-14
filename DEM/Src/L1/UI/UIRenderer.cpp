#include "UIRenderer.h"

#include <Core/Factory.h>
#include <CEGUISystem.h>

namespace Render
{
__ImplementClass(Render::CUIRenderer, 'UIRN', Render::IRenderer);

void CUIRenderer::Render()
{
	CEGUI::System::getSingleton().renderGUI();
}
//---------------------------------------------------------------------

}