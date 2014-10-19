#include "Window.h"

#include <Render/RenderServer.h>
#include <Core/Factory.h>
#include <CEGUIWindowManager.h>

namespace UI
{
__ImplementClass(UI::CUIWindow, 'UIWN', Core::CObject);

void CUIWindow::Init(CEGUI::Window* pWindow)
{
	n_assert(!pWnd && pWindow);
	pWnd = pWindow;
}
//---------------------------------------------------------------------

void CUIWindow::Load(const CString& ResourceFile)
{
	Init(CEGUI::WindowManager::getSingleton().loadWindowLayout(ResourceFile.CStr(), ""));
}
//---------------------------------------------------------------------

vector2 CUIWindow::GetSizeRel()
{
	if (!pWnd) return vector2::zero;

	vector2 ParentSize = GetParentBaseSize(pWnd);
	return vector2(pWnd->getWidth().asRelative(ParentSize.x), pWnd->getHeight().asRelative(ParentSize.y));
}
//---------------------------------------------------------------------

vector2 CUIWindow::GetParentBaseSize(CEGUI::Window* pWindow)
{
	CEGUI::Window* pWndParent = pWindow->getParent();
	if (!pWndParent)
	{
		const CDisplayMode& Disp = RenderSrv->GetDisplay().GetDisplayMode();
		return vector2((float)Disp.Width, (float)Disp.Height);
	}

	vector2 GrandParentSize = GetParentBaseSize(pWndParent);
	return vector2(pWndParent->getWidth().asAbsolute(GrandParentSize.x), pWndParent->getHeight().asAbsolute(GrandParentSize.y));
}
//---------------------------------------------------------------------

}
