#include "LuaConsole.h"

#include <Events/EventManager.h>
#include <Scripting/ScriptServer.h>
#include <Data/Params.h>
#include <Core/Logger.h>

#include <UI/CEGUI/CEGUIFmtLbTextItem.h>
#include <elements/CEGUIEditbox.h>
#include <elements/CEGUIListbox.h>
#include <elements/CEGUIScrollbar.h>

#define MAX_LINES_START	32
#define MAX_LINES		128

namespace Debug
{
__ImplementClass(Debug::CLuaConsole, 'DLUA', Core::CRefCounted); //UI::CWindow);

void CLuaConsole::Init(CEGUI::Window* pWindow)
{
	CWindow::Init(pWindow);

	ConnOnShow = pWnd->subscribeEvent(CEGUI::Window::EventShown, CEGUI::Event::Subscriber(&CLuaConsole::OnShow, this));

	pInputLine = (CEGUI::Editbox*)pWnd->getChild(pWnd->getName() + "/InputLine");
	pInputLine->subscribeEvent(CEGUI::Editbox::EventTextAccepted,
		CEGUI::Event::Subscriber(&CLuaConsole::OnCommand, this));
	pInputLine->subscribeEvent(CEGUI::Editbox::EventKeyDown,
		CEGUI::Event::Subscriber(&CLuaConsole::OnKeyDown, this));
	pInputLine->subscribeEvent(CEGUI::Editbox::EventCharacterKey,
		CEGUI::Event::Subscriber(&CLuaConsole::OnChar, this));
	pInputLine->activate();

	pOutputWnd = (CEGUI::Listbox*)pWnd->getChild(pWnd->getName() + "/OutputList");
	pVertScroll = pOutputWnd->getVertScrollbar();

	nLineBuffer* pLineBuffer = CoreLogger->GetLineBuffer();
	if (pLineBuffer)
	{
		LPCSTR Lines[MAX_LINES_START];
		int Count = pLineBuffer->GetLines(Lines, MAX_LINES_START);
		while (--Count >= 0) Print(Lines[Count], 0xffb0b0b0);
	}

	SUBSCRIBE_PEVENT(OnLogMsg, CLuaConsole, OnLogMsg);
}
//---------------------------------------------------------------------

void CLuaConsole::Term()
{
	UNSUBSCRIBE_EVENT(OnLogMsg);

	if (ConnOnShow.isValid()) ConnOnShow->disconnect();

	if (pWnd && pWnd->getParent())
		pWnd->getParent()->removeChildWindow(pWnd);

	//CWindow::Term();
}
//---------------------------------------------------------------------

void CLuaConsole::Print(LPCSTR pMsg, DWORD Color)
{
	CEGUI::ListboxTextItem* pNewItem;

	if (pOutputWnd->getItemCount() >= MAX_LINES)
	{
		//???can just move from one index to another?
		pNewItem = (CEGUI::FormattedListboxTextItem*)pOutputWnd->getListboxItemFromIndex(0);
		n_assert(pNewItem);
		pOutputWnd->removeItem(pNewItem);
		pNewItem->setText((CEGUI::utf8*)pMsg);
	}
	else
	{
		//!!!NEED TO DELETE AFTER USE!
		pNewItem = n_new(CEGUI::FormattedListboxTextItem(
			(CEGUI::utf8*)pMsg, CEGUI::HTF_WORDWRAP_LEFT_ALIGNED, 0, 0, true, false));
		//pNewItem = n_new(CEGUI::ListboxTextItem((CEGUI::utf8*)pMsg, 0, 0, true, false));
		n_assert(pNewItem);
		pNewItem->setTextParsingEnabled(false);
	}

	pNewItem->setTextColours(CEGUI::colour(Color));

	bool ShouldScroll = (pVertScroll->getScrollPosition() >= pVertScroll->getDocumentSize() - pVertScroll->getPageSize());
	pOutputWnd->addItem(pNewItem);
	if (ShouldScroll) pOutputWnd->ensureItemIsVisible(pOutputWnd->getItemCount());
}
//---------------------------------------------------------------------

bool CLuaConsole::OnLogMsg(const Events::CEventBase& Event)
{
	Data::PParams P = ((const Events::CEvent&)Event).Params;
	DWORD Color = (P->Get<int>(CStrID("Type")) == Core::CLogger::MsgTypeError) ? 0xfff0c0c0 : 0xffb0b0b0;
	Print((LPCSTR)P->Get<PVOID>(CStrID("pMsg")), Color);
	OK;
}
//---------------------------------------------------------------------

bool CLuaConsole::OnShow(const CEGUI::EventArgs& e)
{
	pInputLine->activate();
	OK;
}
//---------------------------------------------------------------------

//!!!Hack, key can change in layout HRD!
bool CLuaConsole::OnChar(const CEGUI::EventArgs& e)
{
	const CEGUI::KeyEventArgs& ke = (const CEGUI::KeyEventArgs&)e;
	return ke.codepoint == 96 || ke.codepoint == 1105; // Grave || ¸
}
//---------------------------------------------------------------------

bool CLuaConsole::OnCommand(const CEGUI::EventArgs& e)
{
	LPCSTR pCmd = pInputLine->getText().c_str();

	if (!pCmd || !*pCmd) OK;

	Print(pCmd, 0xffffffff);

	if (!strcmp(pCmd, "exit")) EventMgr->FireEvent(CStrID("CloseApplication"));
	else if (!strcmp(pCmd, "cls")) pOutputWnd->resetList();
	else if (!strcmp(pCmd, "help"))
	{
		n_printf(	"Debug Lua console.\n"
					" help                           - view this help\n"
					" exit                           - close the application\n"
					" cls                            - clear output window\n"
					" dir <fullname> / ls <fullname> - view contents of a lua table at <fullname>\n"
					"All other input is executed as a Lua script.");
	}
	else if (!strncmp(pCmd, "dir ", 4) || !strncmp(pCmd, "ls ", 3))
	{
		LPCSTR pTable = pCmd + 3;
		while (*pTable == 32) ++pTable;

		CArray<nString> Contents;

		if (ScriptSrv->PlaceOnStack(pTable))
		{
			ScriptSrv->GetTableFieldsDebug(Contents);
			n_printf("----------\n");
			for (int i = 0; i < Contents.GetCount(); ++i)
				Print(Contents[i].CStr(), 0xffb0b0b0);
			n_printf("----------\n");
		}
		else n_printf("Lua table not found\n");
	}
	//else if (!strncmp(pCmd, "cd ", 3))
	//{
	//	// change current table
	//	// pCmd + 3, while 32 ++pCmd
	//}
	else ScriptSrv->RunScript(pCmd);

	if (CmdHistory.GetCount() > 32) CmdHistory.RemoveAt(0);
	CmdHistory.Add(pCmd);
	CmdHistoryCursor = CmdHistory.GetCount();

	pInputLine->setText("");

	OK;
}
//---------------------------------------------------------------------

bool CLuaConsole::OnKeyDown(const CEGUI::EventArgs& e)
{
	const CEGUI::KeyEventArgs& ke = (const CEGUI::KeyEventArgs&)e;

	if (ke.scancode == CEGUI::Key::ArrowDown)
	{
		if (CmdHistory.GetCount())
		{
			if (++CmdHistoryCursor >= CmdHistory.GetCount())
				CmdHistoryCursor = CmdHistory.GetCount() - 1;
			pInputLine->setText((CEGUI::utf8*)CmdHistory[CmdHistoryCursor].CStr());
			pInputLine->setCaratIndex(pInputLine->getText().length());
		}
		OK;
	}
	else if (ke.scancode == CEGUI::Key::ArrowUp)
	{
		if (CmdHistory.GetCount())
		{
			if (--CmdHistoryCursor < 0)
				CmdHistoryCursor = 0;
			pInputLine->setText((CEGUI::utf8*)CmdHistory[CmdHistoryCursor].CStr());
			pInputLine->setCaratIndex(pInputLine->getText().length());
		}
		OK;
	}
	else if (ke.scancode == CEGUI::Key::Grave)
	{
		//++ke.handled;
		OK;
	}

	FAIL;
}
//---------------------------------------------------------------------

}
