#pragma once
#ifndef __DEM_L1_UI_WINDOW_H__
#define __DEM_L1_UI_WINDOW_H__

#include <Core/Object.h>
#include <UI/UIFwd.h>
#include <CEGUI/Window.h>
#include <Math/Vector2.h>
#include <functional>

// CEGUI Window wrapper/controller/extender. Base class for all UI windows.

namespace UI
{

class CUIWindow: public Core::CObject
{
	__DeclareClass(CUIWindow);

protected:

	CEGUI::Window* pWnd = nullptr;

	static vector2 GetParentBaseSize(CEGUI::Window* pWindow);

public:

	CUIWindow() {}
	CUIWindow(const char* pResourceFile) { Load(pResourceFile); }
	virtual ~CUIWindow() {}

	virtual void	Init(CEGUI::Window* pWindow);
	virtual void	Term();
	void			Load(const char* pResourceFile);
	
	bool			IsVisible() const { return pWnd->isVisible(); }
	void			Show() { if (!IsVisible()) SetVisible(true); }
	void			Hide() { if (IsVisible()) SetVisible(false); }
	void			ToggleVisibility() { SetVisible(!IsVisible()); }
	//???virtual or listen OnShow/OnHide and perform custom actions there?
	virtual void	SetVisible(bool Visible) { n_assert(pWnd); pWnd->setVisible(Visible); }
	void			SetDrawMode(EDrawMode Mode);
	
	void			SetPosition(const CEGUI::UVector2& Pos) { n_assert(pWnd); pWnd->setPosition(Pos); }
	void			SetPosition(const vector2& Pos) { n_assert(pWnd); pWnd->setPosition(CEGUI::UVector2(CEGUI::UDim(0.f, Pos.x), CEGUI::UDim(0.f, Pos.y))); }
	void			SetPositionRel(const vector2& Pos) { n_assert(pWnd); pWnd->setPosition(CEGUI::UVector2(CEGUI::UDim(Pos.x, 0.f), CEGUI::UDim(Pos.y, 0.f))); }
	void			SetPositionRel(float x, float y) { n_assert(pWnd); pWnd->setPosition(CEGUI::UVector2(CEGUI::UDim(x, 0.f), CEGUI::UDim(y, 0.f))); }
	vector2			GetSizeRel();
	void			SetFocus() { pWnd->activate(); }

	bool			SubscribeButtonClick(const char* pPath, std::function<void()> Callback);

	CEGUI::Window*	GetWnd() const { return pWnd; }

	inline bool		operator ==(const CUIWindow& Other) const { return this == &Other || (pWnd && pWnd == Other.pWnd); }
};

typedef Ptr<CUIWindow> PUIWindow;

}

#endif