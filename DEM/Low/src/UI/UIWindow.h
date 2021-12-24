#pragma once
#include <Core/Object.h>
#include <UI/UIFwd.h>
#include <CEGUI/Window.h>
#include <Math/Vector2.h>
#include <Input/InputFwd.h>

// Base class for all UI windows

namespace Events
{
	class CEventDispatcher;
}

namespace UI
{

class CUIWindow: public Core::CObject
{
	FACTORY_CLASS_DECL;

protected:

	CEGUI::Window* pWnd = nullptr;

	static vector2 GetParentBaseSize(CEGUI::Window* pWindow);

public:

	CUIWindow() = default;
	CUIWindow(const char* pResourceFile) { Load(pResourceFile); }
	virtual ~CUIWindow() override;

	void			Load(const char* pResourceFile);
	virtual void    Update(float dt) {}
	
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

	bool			SetWidgetEnabled(const char* pPath, bool Enabled);
	bool			SetWidgetText(const char* pPath, const CString& Text);
	bool			SubscribeButtonClick(const char* pPath, std::function<void()> Callback);

	virtual bool	OnAxisMove(Events::CEventDispatcher* /*pDispatcher*/, const Event::AxisMove& /*Event*/) { FAIL; }
	virtual bool	OnButtonDown(Events::CEventDispatcher* /*pDispatcher*/, const Event::ButtonDown& /*Event*/) { FAIL; }
	virtual bool	OnButtonUp(Events::CEventDispatcher* /*pDispatcher*/, const Event::ButtonUp& /*Event*/) { FAIL; }
	virtual bool	OnTextInput(Events::CEventDispatcher* /*pDispatcher*/, const Event::TextInput& /*Event*/) { FAIL; }

	CEGUI::Window*	GetWnd() const { return pWnd; }

	inline bool		operator ==(const CUIWindow& Other) const { return this == &Other || (pWnd && pWnd == Other.pWnd); }
};

typedef Ptr<CUIWindow> PUIWindow;

}
