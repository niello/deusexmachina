#include "UIWindow.h"

#include <Core/Factory.h>
#include <Data/String.h>
#include <CEGUI/WindowManager.h>
#include <CEGUI/CoordConverter.h>
#include <CEGUI/GUIContext.h>
#include <CEGUI/widgets/PushButton.h>

namespace UI
{
FACTORY_CLASS_IMPL(UI::CUIWindow, 'UIWN', Core::CObject);

CUIWindow::~CUIWindow()
{
	CEGUI::WindowManager::getSingleton().destroyWindow(pWnd);
}
//---------------------------------------------------------------------

void CUIWindow::Load(const char* pResourceFile)
{
	n_assert(!pWnd);
	pWnd = CEGUI::WindowManager::getSingleton().loadLayoutFromFile(pResourceFile);
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

bool CUIWindow::SetWidgetEnabled(const char* pPath, bool Enabled)
{
	if (!pPath) FAIL;

	CEGUI::Window* pChild = pWnd->getChild(pPath);
	if (!pChild) FAIL;

	pChild->setEnabled(Enabled);

	OK;
}
//---------------------------------------------------------------------

bool CUIWindow::SetWidgetText(const char* pPath, const CString& Text)
{
	if (!pPath) FAIL;

	CEGUI::Window* pChild = pWnd->getChild(pPath);
	if (!pChild) FAIL;

	pChild->setText(Text.CStr());

	OK;
}
//---------------------------------------------------------------------

// TODO: SubscribeWidgetClick instead, without type restriction?
// Or other widgets don't send EventClicked?
bool CUIWindow::SubscribeButtonClick(const char* pPath, std::function<void()> Callback)
{
	if (!pPath) FAIL;

	CEGUI::Window* pChild = pWnd->getChild(pPath);
	if (!pChild) FAIL;

	// TODO: CEGUI implement - get look-independent window type from CEGUI
	const CEGUI::String& Type = pChild->getType();
	const CEGUI::String WidgetTypeName(Type, Type.find('/') + 1);

	if (WidgetTypeName == "PushButton" || WidgetTypeName == "Button")
	{
		pChild->subscribeEvent(CEGUI::PushButton::EventClicked, Callback);
		OK;
	}

	FAIL;
}
//---------------------------------------------------------------------

}
