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
	CEGUI::uint32 CEGUIDrawMode = 0;
	if (Mode & DrawMode_Opaque) CEGUIDrawMode |= 0x04;
	if (Mode & DrawMode_Transparent) CEGUIDrawMode |= CEGUI::Window::DrawModeFlagWindowRegular;
	if (pWnd) pWnd->setDrawMode(CEGUIDrawMode);
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
