#pragma once
#ifndef __DEM_L1_UI_WINDOW_H__
#define __DEM_L1_UI_WINDOW_H__

#include <Core/Object.h>
#include <CEGUIWindow.h>

// CEGUI Window wrapper/controller/extender. Base class for all UI windows.

namespace UI
{

class CWindow: public Core::CObject
{
	__DeclareClass(CWindow);

protected:

	CEGUI::Window* pWnd;

	static vector2 GetParentBaseSize(CEGUI::Window* pWindow);

public:

	CWindow(): pWnd(NULL) {}
	virtual ~CWindow() {}

	virtual void	Init(CEGUI::Window* pWindow);
	virtual void	Term() {} //!!!if attached to parent, remove. Then unload.
	void			Load(const CString& ResourceFile);
	
	bool			IsVisible() const { return pWnd->isVisible(); }
	void			Show() { if (!IsVisible()) SetVisible(true); }
	void			Hide() { if (IsVisible()) SetVisible(false); }
	void			ToggleVisibility() { SetVisible(!IsVisible()); }
	//???virtual or listen OnShow/OnHide and perform custom actions there?
	virtual void	SetVisible(bool Visible) { n_assert(pWnd); pWnd->setVisible(Visible); }
	
	void			SetPosition(const CEGUI::UVector2& Pos) { n_assert(pWnd); pWnd->setPosition(Pos); }
	void			SetPosition(const vector2& Pos) { n_assert(pWnd); pWnd->setPosition(CEGUI::UVector2(CEGUI::UDim(0.f, Pos.x), CEGUI::UDim(0.f, Pos.y))); }
	void			SetPositionRel(const vector2& Pos) { n_assert(pWnd); pWnd->setPosition(CEGUI::UVector2(CEGUI::UDim(Pos.x, 0.f), CEGUI::UDim(Pos.y, 0.f))); }
	void			SetPositionRel(float x, float y) { n_assert(pWnd); pWnd->setPosition(CEGUI::UVector2(CEGUI::UDim(x, 0.f), CEGUI::UDim(y, 0.f))); }
	vector2			GetSizeRel();
	void			SetFocus() { pWnd->activate(); }

	CEGUI::Window*	GetWnd() const { return pWnd; }
};

typedef Ptr<CWindow> PWindow;

}

#endif