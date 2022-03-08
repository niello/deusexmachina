#include "LuaConsole.h"

#include <Events/EventServer.h>
#include <Scripting/ScriptServer.h>
#include <Data/Params.h>
#include <Core/Factory.h>

#include <CEGUI/widgets/ListboxTextItem.h>
#include <CEGUI/widgets/Editbox.h>
#include <CEGUI/widgets/MultiColumnList.h>
#include <CEGUI/widgets/Scrollbar.h>

#define MAX_LINES_START	32
#define MAX_LINES		128

namespace Debug
{
FACTORY_CLASS_IMPL(Debug::CLuaConsole, 'DLUA', Core::CObject); //UI::CUIWindow);

CLuaConsole::CLuaConsole()
	: CmdHistoryCursor(0)
{
	Init();
}
//---------------------------------------------------------------------

CLuaConsole::~CLuaConsole()
{
	UNSUBSCRIBE_EVENT(OnLogMsg);

	if (ConnOnShow) ConnOnShow->disconnect();
}
//---------------------------------------------------------------------

void CLuaConsole::Init()
{
	ConnOnShow = pWnd->subscribeEvent(CEGUI::Window::EventShown, CEGUI::Event::Subscriber(&CLuaConsole::OnShow, this));

	pInputLine = (CEGUI::Editbox*)pWnd->getChild("InputLine");
	pInputLine->subscribeEvent(CEGUI::Editbox::EventTextAccepted,
		CEGUI::Event::Subscriber(&CLuaConsole::OnCommand, this));
	pInputLine->subscribeEvent(CEGUI::Editbox::EventSemanticEvent,
		CEGUI::Event::Subscriber(&CLuaConsole::OnSemanticEvent, this));
	pInputLine->activate();

	pOutputWnd = (CEGUI::MultiColumnList*)pWnd->getChild("OutputList");
	pVertScroll = pOutputWnd->getVertScrollbar();

	SUBSCRIBE_PEVENT(OnLogMsg, CLuaConsole, OnLogMsg);
}
//---------------------------------------------------------------------

void CLuaConsole::Print(const char* pMsg, U32 ColorARGB)
{
	CEGUI::ListboxTextItem* pNewItem;

	if (pOutputWnd->getRowCount() >= MAX_LINES)
	{
		//???can just move from one index to another?
		pNewItem = (CEGUI::ListboxTextItem*)pOutputWnd->getChildElementAtIndex(0);
		n_assert(pNewItem);
		pOutputWnd->removeRow(0);
		pNewItem->setText(pMsg);
	}
	else
	{
		//!!!NEED TO DELETE AFTER USE!
		pNewItem = n_new(CEGUI::ListboxTextItem(
			pMsg, /*CEGUI::HorizontalTextFormatting::WordWrapLeftAligned,*/ 0, 0, true, false));
		n_assert(pNewItem);
		pNewItem->setTextParsingEnabled(false);
	}

	pNewItem->setTextColours(CEGUI::Colour(ColorARGB));

	//!!!FIXME!
	U32 ColumnID = -1;

	bool ShouldScroll = (pVertScroll->getScrollPosition() >= pVertScroll->getDocumentSize() - pVertScroll->getPageSize());
	pOutputWnd->addRow(pNewItem, ColumnID);
	if (ShouldScroll) pOutputWnd->ensureItemIsVisible(pNewItem);
}
//---------------------------------------------------------------------

bool CLuaConsole::OnLogMsg(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	Data::PParams P = ((const Events::CEvent&)Event).Params;
	U32 ColorARGB = (P->Get<int>(CStrID("Type")) == Sys::MsgType_Error) ? 0xfff0c0c0 : 0xffb0b0b0;
	Print((const char*)P->Get<PVOID>(CStrID("pMsg")), ColorARGB);
	OK;
}
//---------------------------------------------------------------------

bool CLuaConsole::OnShow(const CEGUI::EventArgs& e)
{
	pInputLine->activate();
	OK;
}
//---------------------------------------------------------------------

bool CLuaConsole::OnCommand(const CEGUI::EventArgs& e)
{
	std::string Cmd = CEGUI::String::convertUtf32ToUtf8(pInputLine->getText().c_str());

	if (Cmd.empty()) OK;

	Print(Cmd.c_str(), 0xffffffff);

	if (Cmd == "exit") EventSrv->FireEvent(CStrID("CloseApplication"));
	else if (Cmd == "cls") pOutputWnd->resetList();
	else if (Cmd == "help")
	{
		Sys::Log(	"Debug Lua console.\n"
					" help                           - view this help\n"
					" exit                           - close the application\n"
					" cls                            - clear output window\n"
					" dir <fullname> / ls <fullname> - view contents of a lua table at <fullname>\n"
					"All other input is executed as a Lua script.");
	}
	else if (!strncmp(Cmd.c_str(), "dir ", 4) || !strncmp(Cmd.c_str(), "ls ", 3))
	{
		const char* pTable = Cmd.c_str() + 3;
		while (*pTable == 32) ++pTable;

		CArray<CString> Contents;

		if (ScriptSrv->PlaceOnStack(pTable))
		{
			ScriptSrv->GetTableFieldsDebug(Contents);
			Sys::Log("----------\n");
			for (UPTR i = 0; i < Contents.GetCount(); ++i)
				Print(Contents[i].CStr(), 0xffb0b0b0);
			Sys::Log("----------\n");
		}
		else Sys::Log("Lua table not found\n");
	}
	//else if (!strncmp(pCmd, "cd ", 3))
	//{
	//	// change current table
	//	// pCmd + 3, while 32 ++pCmd
	//}
	else
	{
		Data::CData RetVal;
		ScriptSrv->RunScript(Cmd.c_str(), -1, &RetVal);
		if (RetVal.IsValid()) Sys::Log("Return value: %s\n", RetVal.ToString());
	}

	if (CmdHistoryCursor + 1 != CmdHistory.GetCount())
	{
		if (CmdHistory.GetCount() > 32) CmdHistory.RemoveAt(0);
		CmdHistory.Add(CString(Cmd.c_str()));
	}
	CmdHistoryCursor = CmdHistory.GetCount();

	pInputLine->setText("");

	OK;
}
//---------------------------------------------------------------------

bool CLuaConsole::OnSemanticEvent(const CEGUI::EventArgs& e)
{
	const CEGUI::SemanticEventArgs& ke = static_cast<const CEGUI::SemanticEventArgs&>(e);

	if (ke.d_semanticValue == CEGUI::SemanticValue::GoDown)
	{
		if (CmdHistory.GetCount() > CmdHistoryCursor + 1)
		{
			++CmdHistoryCursor;
			pInputLine->setText(CmdHistory[CmdHistoryCursor].CStr());
			pInputLine->setCaretIndex(pInputLine->getText().length());
		}
		else if (pInputLine->getText().c_str() && *pInputLine->getText().c_str())
		{
			if (CmdHistory.GetCount() == CmdHistoryCursor)
			{
				if (CmdHistory.GetCount() > 32) CmdHistory.RemoveAt(0);
				CmdHistory.Add(CString(CEGUI::String::convertUtf32ToUtf8(pInputLine->getText().c_str()).c_str()));
			}
			CmdHistoryCursor = CmdHistory.GetCount();

			pInputLine->setText("");
		}
		OK;
	}
	else if (ke.d_semanticValue == CEGUI::SemanticValue::GoUp)
	{
		if (CmdHistory.GetCount())
		{
			if (CmdHistoryCursor > 0) --CmdHistoryCursor;
			pInputLine->setText(CmdHistory[CmdHistoryCursor].CStr());
			pInputLine->setCaretIndex(pInputLine->getText().length());
		}
		OK;
	}


	// TODO: CEGUI fix
	/*
	else if (ke.scancode == CEGUI::Key::Scan::Grave)
	{
		//!!!HACK, key can change in a control layout!
		pWnd->hide();
		OK;
	}
	*/

	FAIL;
}
//---------------------------------------------------------------------

}
