#pragma once
#include <Core/Object.h>
#include <UI/UIFwd.h>
#include <Events/Signal.h>
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
using PUIWindow = Ptr<class CUIWindow>;
class CUIContext;

class CUIWindow: public DEM::Core::CObject
{
	FACTORY_CLASS_DECL;

protected:

	friend class CUIContext;

	CEGUI::Window*         pWnd = nullptr;

	CUIContext*            _pCtx = nullptr;
	CUIWindow*             _pParent = nullptr;

	std::vector<PUIWindow> _ChildWindows;

	static vector2 GetParentBaseSize(CEGUI::Window* pWindow);

	void OnAttachedToContext(CUIContext* pCtx);
	void OnDetachedFromContext();
	void OnReleasedByOwner(PUIWindow&& Self);

public:

	DEM::Events::CSignal<void()>               OnClosed;
	DEM::Events::CSignal<void(PUIWindow Self)> OnOrphaned;

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
	vector2			GetSizeRel() const;
	void			SetFocus();

	bool			SetWidgetEnabled(const char* pPath, bool Enabled);
	bool			SetWidgetText(const char* pPath, std::string_view Text);
	bool			SubscribeButtonClick(const char* pPath, std::function<void()> Callback);

	void            AddChild(CUIWindow* pChildWindow);
	void            RemoveChild(CUIWindow* pChildWindow);
	void            Close();

	virtual bool	OnAxisMove(Events::CEventDispatcher* /*pDispatcher*/, const Event::AxisMove& /*Event*/) { FAIL; }
	virtual bool	OnButtonDown(Events::CEventDispatcher* /*pDispatcher*/, const Event::ButtonDown& /*Event*/) { FAIL; }
	virtual bool	OnButtonUp(Events::CEventDispatcher* /*pDispatcher*/, const Event::ButtonUp& /*Event*/) { FAIL; }
	virtual bool	OnTextInput(Events::CEventDispatcher* /*pDispatcher*/, const Event::TextInput& /*Event*/) { FAIL; }

	CEGUI::Window*  GetCEGUIWindow() const { return pWnd; }
	CUIWindow*	    GetParent() const { return _pParent; }
	CUIContext*	    GetContext() const { return _pCtx; }

	inline bool		operator ==(const CUIWindow& Other) const { return this == &Other || (pWnd && pWnd == Other.pWnd); }
};

}
