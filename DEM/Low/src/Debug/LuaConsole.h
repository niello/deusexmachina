#pragma once
#include <UI/UIWindow.h>
#include <Events/EventsFwd.h>
#include <Data/Array.h>

// Lua console window, that allows to see engine output and to execute Lua commands.

namespace CEGUI
{
	class Editbox;
	class MultiColumnList;
	class Scrollbar;
	class EventArgs;
}

namespace Debug
{

class CLuaConsole: public UI::CUIWindow
{
	FACTORY_CLASS_DECL;

protected:

	CEGUI::Editbox*				pInputLine;
	CEGUI::MultiColumnList*		pOutputWnd;
	CEGUI::Scrollbar*			pVertScroll;

	CEGUI::Event::Connection	ConnOnShow;

	std::vector<CString>				CmdHistory;
	UPTR						CmdHistoryCursor;

	void Init();
	void Print(const char* pMsg, U32 ColorARGB);

	bool OnShow(const CEGUI::EventArgs& e);
	bool OnCommand(const CEGUI::EventArgs& e);
	bool OnKeyDown(const CEGUI::EventArgs& e);

	DECLARE_EVENT_HANDLER(OnLogMsg, OnLogMsg);

public:

	CLuaConsole();
	virtual ~CLuaConsole() override;

	void	SetInputFocus() { ((CEGUI::Window*)pInputLine)->activate(); }
};

typedef Ptr<CLuaConsole> PLuaConsole;

}
