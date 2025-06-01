#include "UIWindow.h"

#include <Core/Factory.h>
#include <UI/UIContext.h>
#include <CEGUI/WindowManager.h>
#include <CEGUI/CoordConverter.h>
#include <CEGUI/GUIContext.h>
#include <CEGUI/widgets/PushButton.h>

namespace UI
{
FACTORY_CLASS_IMPL(UI::CUIWindow, 'UIWN', DEM::Core::CObject);

CUIWindow::~CUIWindow()
{
	n_assert_dbg(!_pCtx && !_pParent);

	auto ChildWindows = std::move(_ChildWindows);
	for (auto& ChildWnd : ChildWindows)
	{
		if (_pCtx) ChildWnd->OnDetachedFromContext();
		ChildWnd->OnReleasedByOwner(std::move(ChildWnd));
	}

	CEGUI::WindowManager::getSingleton().destroyWindow(pWnd);
}
//---------------------------------------------------------------------

void CUIWindow::Load(const char* pResourceFile)
{
	n_assert(!pWnd);
	pWnd = CEGUI::WindowManager::getSingleton().loadLayoutFromFile(pResourceFile);
	pWnd->setDestroyedByParent(false); // We control our window and don't want it to be occasionally deleted from outside
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

vector2 CUIWindow::GetSizeRel() const
{
	if (!pWnd) return vector2::zero;

	vector2 ParentSize = GetParentBaseSize(pWnd);
	return vector2(CEGUI::CoordConverter::asRelative(pWnd->getWidth(), ParentSize.x),
				   CEGUI::CoordConverter::asRelative(pWnd->getHeight(), ParentSize.y));
}
//---------------------------------------------------------------------

void CUIWindow::SetFocus()
{
	// FIXME CEGUI: DefaultWindow can't be activated without a window renderer even if it has children with visual appearance
	if (pWnd->activate()) return;

	for (size_t i = 0; i < pWnd->getChildCount(); ++i)
		if (pWnd->getChildAtIndex(i)->activate()) return;
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

bool CUIWindow::SetWidgetText(const char* pPath, std::string_view Text)
{
	if (!pPath) FAIL;

	CEGUI::Window* pChild = pWnd->getChild(pPath);
	if (!pChild) FAIL;

	// FIXME: support string views in CEGUI!
	pChild->setText(std::string(Text));

	OK;
}
//---------------------------------------------------------------------

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

void CUIWindow::AddChild(CUIWindow* pChildWindow)
{
	n_assert_dbg(pChildWindow && !pChildWindow->_pParent && !pChildWindow->_pCtx);

	if (!pChildWindow || pChildWindow->_pParent || pChildWindow->_pCtx) return;

	auto It = std::find(_ChildWindows.cbegin(), _ChildWindows.cend(), pChildWindow);
	if (It != _ChildWindows.cend()) return;

	_ChildWindows.push_back(pChildWindow);

	pWnd->addChild(pChildWindow->GetCEGUIWindow());

	if (_pCtx) pChildWindow->OnAttachedToContext(_pCtx);
}
//---------------------------------------------------------------------

void CUIWindow::RemoveChild(CUIWindow* pChildWindow)
{
	if (!pChildWindow) return;

	auto It = std::find(_ChildWindows.begin(), _ChildWindows.end(), pChildWindow);
	if (It == _ChildWindows.cend()) return;

	auto ChildWnd = std::move(*It);
	pWnd->removeChild(ChildWnd->GetCEGUIWindow());

	_ChildWindows.erase(It);

	if (_pCtx) ChildWnd->OnDetachedFromContext();
	ChildWnd->OnReleasedByOwner(std::move(ChildWnd));
}
//---------------------------------------------------------------------

void CUIWindow::Close()
{
	if (_pParent)
		_pParent->RemoveChild(this);
	else if (_pCtx)
		_pCtx->RemoveRootWindow(this);
}
//---------------------------------------------------------------------

void CUIWindow::OnAttachedToContext(CUIContext* pCtx)
{
	n_assert_dbg(!_pCtx && pCtx);

	_pCtx = pCtx;

	// TODO: signal?

	for (auto& ChildWnd : _ChildWindows)
		ChildWnd->OnAttachedToContext(pCtx);
}
//---------------------------------------------------------------------

void CUIWindow::OnDetachedFromContext()
{
	n_assert_dbg(_pCtx);

	for (auto& ChildWnd : _ChildWindows)
		ChildWnd->OnDetachedFromContext();

	OnClosed();

	_pCtx = nullptr;
}
//---------------------------------------------------------------------

void CUIWindow::OnReleasedByOwner(PUIWindow&& Self)
{
	n_assert_dbg(!_pCtx);

	_pParent = nullptr;

	// If no one claims the ownership in a signal handler, the window may be destroyed after leaving OnReleasedByOwner()
	PUIWindow PossiblyLastRef = std::move(Self);
	OnOrphaned(std::move(PossiblyLastRef));
}
//---------------------------------------------------------------------

}
