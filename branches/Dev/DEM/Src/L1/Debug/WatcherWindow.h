#pragma once
#ifndef __DEM_L1_DBG_WATCHER_WINDOW_H__
#define __DEM_L1_DBG_WATCHER_WINDOW_H__

#include <UI/Window.h>
#include <Events/EventsFwd.h>
#include <Events/Subscription.h>
#include <Data/SimpleString.h>

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

class CWatcherWindow: public UI::CWindow
{
	__DeclareClass(CWatcherWindow);

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
		Data::CSimpleString		VarName;
		int						RowID;
		CEGUI::ListboxTextItem*	pNameItem;
		CEGUI::ListboxTextItem*	pTypeItem;
		CEGUI::ListboxTextItem*	pValueItem;

		CWatched(): RowID(-1), pNameItem(NULL), pTypeItem(NULL), pValueItem(NULL) {}
		//~CWatched() { Clear(); } - leave it commented while tere is no copy constructor which clones text items
		void Clear();
	};

	CArray<CWatched>			Watched;

	bool OnClearClick(const CEGUI::EventArgs& e);
	bool OnAddVarsClick(const CEGUI::EventArgs& e);
	bool OnNewWatchedAccept(const CEGUI::EventArgs& e);
	bool OnListKeyDown(const CEGUI::EventArgs& e);

	DECLARE_EVENT_HANDLER(OnUIUpdate, OnUIUpdate);

public:

	//CWatcherWindow() {}
	virtual ~CWatcherWindow() { for (int i = 0; i < Watched.GetCount(); ++i) Watched[i].Clear(); }

	virtual void	Init(CEGUI::Window* pWindow);
	virtual void	Term();
	virtual void	SetVisible(bool Visible);
	void			SetInputFocus() { ((CEGUI::Window*)pPatternEdit)->activate(); }

	void			AddWatched(EVarType Type, LPCSTR Name);
	void			AddAllGlobals();
};

typedef Ptr<CWatcherWindow> PWatcherWindow;

}

#endif