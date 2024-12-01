#include "UIContext.h"

#include <UI/CEGUI/DEMRenderer.h>
#include <Events/EventDispatcher.h>
#include <Input/InputEvents.h>
#include <System/OSWindow.h>
#include <System/SystemEvents.h>

#include <CEGUI/RenderTarget.h>
#include <CEGUI/System.h>
#include <CEGUI/ImageManager.h>
#include <CEGUI/GUIContext.h>

namespace UI
{

CUIContext::CUIContext(float Width, float Height, DEM::Sys::COSWindow* pHostWindow)
{
	if (!Width || !Height) return;

	auto pRenderer = static_cast<CEGUI::CDEMRenderer*>(CEGUI::System::getSingleton().getRenderer());
	CEGUI::RenderTarget* pTarget = pRenderer->createViewportTarget(Width, Height);
	pCtx = &CEGUI::System::getSingleton().createGUIContext(*pTarget);
	pCtx->initDefaultInputSemantics();

	if (pHostWindow)
	{
		OSWindow = pHostWindow;

		// FIXME: CEGUI doesn't calculate proper font size at the start
		// FIXME: must be per-context, not global!
		const Data::CRect& WndRect = pHostWindow->GetRect();
		const CEGUI::Sizef RectSize(static_cast<float>(WndRect.W), static_cast<float>(WndRect.H));
		CEGUI::System::getSingleton().notifyDisplaySizeChanged(RectSize);

		// FIXME: work with the frame view instead of the OS window?! View may occupy only a part of a window.
		DISP_SUBSCRIBE_PEVENT(pHostWindow, OnFocusSet, CUIContext, OnFocusSet);
		DISP_SUBSCRIBE_PEVENT(pHostWindow, OnToggleFullscreen, CUIContext, OnViewportSizeChanged);
		DISP_SUBSCRIBE_NEVENT(pHostWindow, OSWindowResized, CUIContext, OnViewportSizeChanged);
	}
}
//---------------------------------------------------------------------

CUIContext::~CUIContext()
{
	ClearRootWindowStack();

	if (pCtx)
		CEGUI::System::getSingleton().destroyGUIContext(*pCtx);
}
//---------------------------------------------------------------------

void CUIContext::Update(float dt)
{
	if (pCtx) pCtx->injectTimePulse(dt);

	if (auto pRootWindow = GetRootWindow())
		pRootWindow->Update(dt);
}
//---------------------------------------------------------------------

bool CUIContext::SubscribeOnInput(Events::CEventDispatcher* pDispatcher, U16 Priority)
{
	if (!pDispatcher || !pCtx) FAIL;

	auto& InputSubs = _InputSubs[pDispatcher];
	InputSubs.push_back(pDispatcher->Subscribe(&Event::AxisMove::RTTI, this, &CUIContext::OnAxisMove, Priority));
	InputSubs.push_back(pDispatcher->Subscribe(&Event::ButtonDown::RTTI, this, &CUIContext::OnButtonDown, Priority));
	InputSubs.push_back(pDispatcher->Subscribe(&Event::ButtonUp::RTTI, this, &CUIContext::OnButtonUp, Priority));
	InputSubs.push_back(pDispatcher->Subscribe(&Event::TextInput::RTTI, this, &CUIContext::OnTextInput, Priority));

	OK;
}
//---------------------------------------------------------------------

void CUIContext::UnsubscribeFromInput(Events::CEventDispatcher* pDispatcher)
{
	if (pDispatcher)
		_InputSubs.erase(pDispatcher);
	else
		_InputSubs.clear();
}
//---------------------------------------------------------------------

void CUIContext::SetRootWindow(CUIWindow* pWindow)
{
	if (!pCtx) return;

	pCtx->setRootWindow(pWindow ? pWindow->GetCEGUIWindow() : nullptr);
}
//---------------------------------------------------------------------

void CUIContext::DetachRootWindow(PUIWindow&& Window)
{
	Window->OnDetachedFromContext();
	Window->OnReleasedByOwner(std::move(Window));
}
//---------------------------------------------------------------------

void CUIContext::PushRootWindow(CUIWindow* pWindow)
{
	n_assert_dbg(pWindow && !pWindow->_pParent && !pWindow->_pCtx);

	if (!pWindow || pWindow->_pParent || pWindow->_pCtx) return;

	auto It = std::find(RootWindows.cbegin(), RootWindows.cend(), pWindow);
	if (It != RootWindows.cend()) return;

	RootWindows.push_back(pWindow);

	pWindow->OnAttachedToContext(this);

	SetRootWindow(pWindow);
}
//---------------------------------------------------------------------

PUIWindow CUIContext::PopRootWindow()
{
	if (RootWindows.empty()) return nullptr;

	PUIWindow CurrWnd = std::move(RootWindows.back());
	DetachRootWindow(PUIWindow(CurrWnd)); // NB: if using unique_ptr, orphan handler may steal return value of PopRootWindow!

	RootWindows.pop_back();
	SetRootWindow(GetRootWindow());

	return CurrWnd;
}
//---------------------------------------------------------------------

void CUIContext::RemoveRootWindow(CUIWindow* pWindow)
{
	auto It = std::find(RootWindows.begin(), RootWindows.end(), pWindow);
	if (It == RootWindows.cend()) return;

	// Detach the root from CEGUI before destroying it
	if (pWindow == RootWindows.back()) SetRootWindow(nullptr);

	DetachRootWindow(std::move(*It));

	RootWindows.erase(It);
	SetRootWindow(GetRootWindow());
}
//---------------------------------------------------------------------

void CUIContext::ClearRootWindowStack()
{
	SetRootWindow(nullptr);
	for (auto& Window : RootWindows)
		DetachRootWindow(std::move(Window));
	RootWindows.clear();
}
//---------------------------------------------------------------------

CUIWindow* CUIContext::GetRootWindow() const
{
	return RootWindows.empty() ? nullptr : RootWindows.back().Get();
}
//---------------------------------------------------------------------

void CUIContext::ShowGUI()
{
	if (!pCtx) return;
	CEGUI::Window* pRoot = pCtx->getRootWindow();
	if (pRoot) pRoot->setVisible(true);
}
//---------------------------------------------------------------------

void CUIContext::HideGUI()
{
	if (!pCtx) return;
	CEGUI::Window* pRoot = pCtx->getRootWindow();
	if (pRoot) pRoot->setVisible(false);
}
//---------------------------------------------------------------------

void CUIContext::ShowMouseCursor()
{
	if (pCtx) pCtx->setCursorVisible(true);
}
//---------------------------------------------------------------------

void CUIContext::HideMouseCursor()
{
	if (pCtx) pCtx->setCursorVisible(false);
}
//---------------------------------------------------------------------

void CUIContext::SetMouseCursor(const char* pImageName)
{
	if (!pCtx) return;

	if (pImageName && *pImageName)
		pCtx->setCursorImage(&CEGUI::ImageManager::getSingleton().get(pImageName));
	else
		pCtx->setCursorImage(pCtx->getDefaultCursorImage());
}
//---------------------------------------------------------------------

void CUIContext::SetDefaultMouseCursor(const char* pImageName)
{
	if (pCtx) pCtx->setDefaultCursorImage(pImageName);
}
//---------------------------------------------------------------------

bool CUIContext::GetCursorPosition(float& X, float& Y) const
{
	if (pCtx)
	{
		glm::vec2 CursorPos = pCtx->getCursorPosition();
		X = CursorPos.x;
		Y = CursorPos.y;
		OK;
	}
	FAIL;
}
//---------------------------------------------------------------------

bool CUIContext::GetCursorPositionRel(float& X, float& Y) const
{
	if (pCtx)
	{
		const auto& CtxSize = pCtx->getSurfaceSize();
		glm::vec2 CursorPos = pCtx->getCursorPosition();
		X = CursorPos.x / CtxSize.d_width;
		Y = CursorPos.y / CtxSize.d_height;
		OK;
	}
	FAIL;
}
//---------------------------------------------------------------------

bool CUIContext::IsMouseOverGUI() const
{
	if (!pCtx) return false;

	CEGUI::Window* pMouseWnd = pCtx->getWindowContainingCursor();
	if (!pMouseWnd) return false;

	if (pMouseWnd != pCtx->getRootWindow()) return true;

	// FIXME: root window is always considered containing cursor although it is incorrect!
	return !pMouseWnd->isCursorPassThroughEnabled() && pMouseWnd->isHit(pCtx->getCursorPosition());
}
//---------------------------------------------------------------------

bool CUIContext::OnAxisMove(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	if (!OSWindow || !OSWindow->HasInputFocus()) FAIL;

	const Event::AxisMove& Ev = static_cast<const Event::AxisMove&>(Event);

	if (!RootWindows.empty() && RootWindows.back()->OnAxisMove(pDispatcher, Ev)) OK;

	if (Ev.Device->GetType() == Input::Device_Mouse)
	{
		switch (Ev.Code)
		{
			case 0:
			case 1:
			{
				IPTR X, Y;
				return OSWindow && OSWindow->GetCursorPosition(X, Y) &&
					pCtx->injectMousePosition((float)X, (float)Y);
			}

			case 2:
			case 3:
				return pCtx->injectMouseWheelChange(Ev.Amount);
		}
	}

	FAIL;
}
//---------------------------------------------------------------------

bool CUIContext::OnButtonDown(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	if (!OSWindow || !OSWindow->HasInputFocus()) FAIL;

	const Event::ButtonDown& Ev = static_cast<const Event::ButtonDown&>(Event);

	if (!RootWindows.empty() && RootWindows.back()->OnButtonDown(pDispatcher, Ev)) OK;

	if (Ev.Device->GetType() == Input::Device_Mouse)
	{
		switch (Ev.Code)
		{
			case 0:		return pCtx->injectMouseButtonDown(CEGUI::MouseButton::Left);
			case 1:		return pCtx->injectMouseButtonDown(CEGUI::MouseButton::Right);
			case 2:		return pCtx->injectMouseButtonDown(CEGUI::MouseButton::Middle);
			case 3:		return pCtx->injectMouseButtonDown(CEGUI::MouseButton::X1);
			case 4:		return pCtx->injectMouseButtonDown(CEGUI::MouseButton::X2);
			default:	FAIL;
		}
	}
	else if (Ev.Device->GetType() == Input::Device_Keyboard)
	{
		// NB: DEM keyboard key codes match CEGUI scancodes, so no additional mapping is required
		return pCtx->injectKeyDown((CEGUI::Key::Scan)Ev.Code);
	}

	FAIL;
}
//---------------------------------------------------------------------

bool CUIContext::OnButtonUp(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	if (!OSWindow || !OSWindow->HasInputFocus()) FAIL;

	const Event::ButtonUp& Ev = static_cast<const Event::ButtonUp&>(Event);

	if (!RootWindows.empty() && RootWindows.back()->OnButtonUp(pDispatcher, Ev)) OK;

	if (Ev.Device->GetType() == Input::Device_Mouse)
	{
		switch (Ev.Code)
		{
			case 0:		return pCtx->injectMouseButtonUp(CEGUI::MouseButton::Left);
			case 1:		return pCtx->injectMouseButtonUp(CEGUI::MouseButton::Right);
			case 2:		return pCtx->injectMouseButtonUp(CEGUI::MouseButton::Middle);
			case 3:		return pCtx->injectMouseButtonUp(CEGUI::MouseButton::X1);
			case 4:		return pCtx->injectMouseButtonUp(CEGUI::MouseButton::X2);
			default:	FAIL;
		}
	}
	else if (Ev.Device->GetType() == Input::Device_Keyboard)
	{
		// NB: DEM keyboard key codes match CEGUI scancodes, so no additional mapping is required
		return pCtx->injectKeyUp((CEGUI::Key::Scan)Ev.Code);
	}

	FAIL;
}
//---------------------------------------------------------------------

bool CUIContext::OnTextInput(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	if (!OSWindow || !OSWindow->HasInputFocus()) FAIL;

	const Event::TextInput& Ev = static_cast<const Event::TextInput&>(Event);

	if (!RootWindows.empty() && RootWindows.back()->OnTextInput(pDispatcher, Ev)) OK;

	bool Handled = false;
	for (char Char : Ev.Text)
		Handled |= pCtx->injectChar(Char);
	return Handled;
}
//---------------------------------------------------------------------

bool CUIContext::OnFocusSet(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	// Must clear modifiers released while out of focus. Ignore pressed ones.
	//TODO CEGUI: pCtx->setModifierKeys(false, false, false); / pCtx->resetInputState()!
	return false;
}
//---------------------------------------------------------------------

bool CUIContext::OnViewportSizeChanged(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	const auto& Ev = static_cast<const Event::OSWindowResized&>(Event);
	if (Ev.ManualResizingInProgress) FAIL;

	if (!pCtx || !OSWindow) FAIL;

	const Data::CRect& WndRect = OSWindow->GetRect();
	const CEGUI::Sizef RectSize(static_cast<float>(WndRect.W), static_cast<float>(WndRect.H));

	// FIXME: fonts & images scale based on it, so we must call it for now, but the display size
	//        doesn't change here actually, it is a render target size change, context-local!
	CEGUI::System::getSingleton().notifyDisplaySizeChanged(RectSize);

	auto& RT = pCtx->getRenderTarget();
	CEGUI::Rectf GUIArea(RT.getArea());
	GUIArea.setSize(RectSize);
	RT.setArea(GUIArea);

	IPTR X, Y;
	if (OSWindow->GetCursorPosition(X, Y))
		pCtx->injectMousePosition(static_cast<float>(X), static_cast<float>(Y));

	OK;
}
//---------------------------------------------------------------------

}
