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

	// Set the window transparent by default, it guarantees correct rendering
	//if (pWnd) pWnd->setDrawMode(((U32)DrawMode_Transparent) | CEGUI::Window::DrawModeFlagWindowRegular);
}
//---------------------------------------------------------------------

void CUIWindow::Load(const char* pResourceFile)
{
	Init(CEGUI::WindowManager::getSingleton().loadLayoutFromFile(pResourceFile));
}
//---------------------------------------------------------------------

void CUIWindow::SetDrawMode(EDrawMode Mode)
{
	if (pWnd) pWnd->setDrawMode(((U32)Mode) | CEGUI::Window::DrawModeFlagWindowRegular);
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
n_assert(false);
return vector2();
//		const CDisplayMode& Disp = RenderSrv->GetDisplay().GetDisplayMode();
//		return vector2((float)Disp.Width, (float)Disp.Height);
	}

	vector2 GrandParentSize = GetParentBaseSize(pWndParent);
	return vector2(CEGUI::CoordConverter::asAbsolute(pWndParent->getWidth(), GrandParentSize.x),
				   CEGUI::CoordConverter::asAbsolute(pWndParent->getHeight(), GrandParentSize.y));
}
//---------------------------------------------------------------------

}
