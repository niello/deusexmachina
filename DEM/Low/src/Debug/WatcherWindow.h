#pragma once
#include <UI/UIWindow.h>
#include <Events/EventsFwd.h>
#include <Data/String.h>

// Nebula variable watcher with pattern-matching filter

// Can add capability to add and remove vars to the list by user

namespace CEGUI
{
	class Editbox;
	class MultiColumnList;
	class ListboxTextItem;
	class EventArgs;
}

namespace Debug
{

class CWatcherWindow: public UI::CUIWindow
{
	FACTORY_CLASS_DECL;

public:

	enum EVarType
	{
		DEM = 0,
		Lua
	};

protected:

	CEGUI::Editbox*				pPatternEdit;
	CEGUI::Editbox*				pNewWatchEdit;
	CEGUI::MultiColumnList*		pList;

	struct CWatched
	{
		EVarType				Type;
		CString					VarName;
		int						RowID;
		CEGUI::ListboxTextItem*	pNameItem;
		CEGUI::ListboxTextItem*	pTypeItem;
		CEGUI::ListboxTextItem*	pValueItem;

		CWatched(): RowID(-1), pNameItem(nullptr), pTypeItem(nullptr), pValueItem(nullptr) {}
		//~CWatched() { Clear(); } - leave it commented while tere is no copy constructor which clones text items
		void Clear();
	};

	std::vector<CWatched>			Watched;

	void Init();

	bool OnClearClick(const CEGUI::EventArgs& e);
	bool OnAddVarsClick(const CEGUI::EventArgs& e);
	bool OnNewWatchedAccept(const CEGUI::EventArgs& e);
	bool OnKeyDown(const CEGUI::EventArgs& e);

	DECLARE_EVENT_HANDLER(OnUIUpdate, OnUIUpdate);

public:

	CWatcherWindow();
	virtual ~CWatcherWindow() override;

	virtual void	SetVisible(bool Visible);
	void			SetInputFocus() { ((CEGUI::Window*)pPatternEdit)->activate(); }

	void			AddWatched(EVarType Type, const char* Name);
	void			AddAllGlobals();
};

typedef Ptr<CWatcherWindow> PWatcherWindow;

}
