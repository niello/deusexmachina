#include "UIWindow.h"

#include <Core/Factory.h>
#include <CEGUI/WindowManager.h>
#include <CEGUI/CoordConverter.h>

namespace UI
{
__ImplementClass(UI::CUIWindow, 'UIWN', Core::CObject);

void CUIWindow::Init(CEGUI::Window* pWindow)
{
	n_assert(!pWnd && pWindow);
	pWnd = pWindow;
	//if (pWnd) pWnd->setDrawMode(CEGUI::Window::DrawModeFlagWindowRegular);
}
//---------------------------------------------------------------------

void CUIWindow::Term()
{
	if (pWnd && pWnd->getParent())
		pWnd->getParent()->removeChild(pWnd);
	//???unload / delete?
}
//---------------------------------------------------------------------

void CUIWindow::Load(const char* pResourceFile)
{
	Init(CEGUI::WindowManager::getSingleton().loadLayoutFromFile(pResourceFile));
}
//---------------------------------------------------------------------

void CUIWindow::SetDrawMode(EDrawMode Mode)
{
	unsigned int CEGUIDrawMode = 0;
	if (Mode & DrawMode_Opaque) CEGUIDrawMode |= DrawModeFlagWindowOpaque;
	if (Mode & DrawMode_Transparent) CEGUIDrawMode |= CEGUI::DrawModeFlagWindowRegular;
	if (pWnd) pWnd->setDrawModeMask(CEGUIDrawMode);
}
//---------------------------------------------------------------------

vector2 CUIWindow::GetSizeRel()
{
	if (!pWnd) return vector2::zero;

	vector2 ParentSize = GetParentBaseSize(pWnd);
	return vector2(CEGUI::CoordConverter::asRelative(pWnd->getWidth(), ParentSize.x),
				   CEGUI::CoordConverter::asRelative(pWnd->getHeight(), ParentSize.y));
}
//---------------------------------------------------------------------

vector2 CUIWindow::GetParentBaseSize(CEGUI::Window* pWindow)
{
	CEGUI::Window* pWndParent = pWindow->getParent();
	if (!pWndParent)
	{
		const CEGUI::Sizef& ContextSize = pWindow->getGUIContext().getSurfaceSize();
		return vector2(ContextSize.d_width, ContextSize.d_height);
	}

	vector2 GrandParentSize = GetParentBaseSize(pWndParent);
	return vector2(CEGUI::CoordConverter::asAbsolute(pWndParent->getWidth(), GrandParentSize.x),
				   CEGUI::CoordConverter::asAbsolute(pWndParent->getHeight(), GrandParentSize.y));
}
//---------------------------------------------------------------------

}
