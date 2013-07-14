#include "WatcherWindow.h"

#include <Scripting/ScriptServer.h>
#include <Events/EventManager.h>
#include <Data/DataArray.h>
#include <Core/CoreServer.h>

#include <elements/CEGUIEditbox.h>
#include <elements/CEGUIMultiColumnList.h>
#include <elements/CEGUIListboxTextItem.h>
#include <elements/CEGUIRadioButton.h>
#include <elements/CEGUIPushButton.h>

#define COL_NAME	0
#define COL_TYPE	1
#define COL_VALUE	2

namespace Debug
{
__ImplementClass(Debug::CWatcherWindow, 'DWWW', UI::CWindow);

using namespace Data;

void CWatcherWindow::Init(CEGUI::Window* pWindow)
{
	CWindow::Init(pWindow);

	pPatternEdit = (CEGUI::Editbox*)pWnd->getChild(pWnd->getName() + "/PatternEdit");

	pList = (CEGUI::MultiColumnList*)pWnd->getChild(pWnd->getName() + "/List");
	pList->addColumn("Name", COL_NAME, CEGUI::UDim(0, 300));
	pList->addColumn("Type", COL_TYPE, CEGUI::UDim(0, 100));
	pList->addColumn("Value", COL_VALUE, CEGUI::UDim(0, 200));
	pList->setSelectionMode(CEGUI::MultiColumnList::RowSingle);
	pList->subscribeEvent(CEGUI::MultiColumnList::EventKeyDown,
		CEGUI::Event::Subscriber(&CWatcherWindow::OnListKeyDown, this));

	CEGUI::RadioButton* pRBNEnv = (CEGUI::RadioButton*)pWnd->getChild(pWnd->getName() + "/RBNEnv");
	pRBNEnv->setGroupID(0);

	CEGUI::RadioButton* pRBLua = (CEGUI::RadioButton*)pWnd->getChild(pWnd->getName() + "/RBLua");
	pRBLua->setGroupID(0);

	pRBNEnv->setSelected(true);

	pNewWatchEdit = (CEGUI::Editbox*)pWnd->getChild(pWnd->getName() + "/NewWatchEdit");
	pNewWatchEdit->subscribeEvent(CEGUI::Editbox::EventTextAccepted,
		CEGUI::Event::Subscriber(&CWatcherWindow::OnNewWatchedAccept, this));

	pWnd->getChild(pWnd->getName() + "/BtnClear")->subscribeEvent(CEGUI::PushButton::EventClicked,
		CEGUI::Event::Subscriber(&CWatcherWindow::OnClearClick, this));

	pWnd->getChild(pWnd->getName() + "/BtnAllVars")->subscribeEvent(CEGUI::PushButton::EventClicked,
		CEGUI::Event::Subscriber(&CWatcherWindow::OnAddVarsClick, this));

	if (IsVisible()) SUBSCRIBE_PEVENT(OnUIUpdate, CWatcherWindow, OnUIUpdate);
}
//---------------------------------------------------------------------

void CWatcherWindow::Term()
{
	UNSUBSCRIBE_EVENT(OnUIUpdate);

	if (pWnd && pWnd->getParent())
		pWnd->getParent()->removeChildWindow(pWnd);

	//CWindow::Term();
}
//---------------------------------------------------------------------

void CWatcherWindow::SetVisible(bool Visible)
{
	if (Visible) SUBSCRIBE_PEVENT(OnUIUpdate, CWatcherWindow, OnUIUpdate);
	else UNSUBSCRIBE_EVENT(OnUIUpdate);
	CWindow::SetVisible(Visible);
}
//---------------------------------------------------------------------

void CWatcherWindow::AddWatched(EVarType Type, LPCSTR Name)
{
	CWatched& Curr = *Watched.Reserve(1);
	Curr.Type = Type;
	Curr.VarName = Name;

	if (!Curr.pNameItem)
	{
		Curr.pNameItem = n_new(CEGUI::ListboxTextItem((CEGUI::utf8*)Name, 0, 0, false, false));
		Curr.pNameItem->setSelectionBrushImage("TaharezLook", "MultiListSelectionBrush");
		Curr.pNameItem->setSelectionColours(CEGUI::colour(0xff606099));
		Curr.pNameItem->setTextParsingEnabled(false);
	}
	else Curr.pNameItem->setText((CEGUI::utf8*)Name);

	if (!Curr.pTypeItem)
	{
		Curr.pTypeItem = n_new(CEGUI::ListboxTextItem("", 0, 0, false, false));
		Curr.pTypeItem->setSelectionBrushImage("TaharezLook", "MultiListSelectionBrush");
		Curr.pTypeItem->setSelectionColours(CEGUI::colour(0xff606099));
		Curr.pTypeItem->setTextParsingEnabled(false);
	}

	if (!Curr.pValueItem)
	{
		Curr.pValueItem = n_new(CEGUI::ListboxTextItem("", 0, 0, false, false));
		Curr.pValueItem->setSelectionBrushImage("TaharezLook", "MultiListSelectionBrush");
		Curr.pValueItem->setSelectionColours(CEGUI::colour(0xff606099));
		Curr.pValueItem->setTextParsingEnabled(false);
	}
}
//---------------------------------------------------------------------

void CWatcherWindow::AddAllGlobals()
{
	int i = Watched.GetCount();
	for (CHashMap<CData>::CIterator It = CoreSrv->Globals.Begin(); !It.IsEnd(); ++It, ++i)
	{
		nString Str(It.GetKey(), It.GetKeyLength());
		AddWatched(DEM, Str.CStr());
	}

	for (int j = i; j < Watched.GetCount(); ++j)
		Watched[j].Clear();
	Watched.Resize(i);
}
//---------------------------------------------------------------------

bool CWatcherWindow::OnNewWatchedAccept(const CEGUI::EventArgs& e)
{
	LPCSTR pName = pNewWatchEdit->getText().c_str();
	if (!pName || !*pName) OK;

	CEGUI::RadioButton* pRBNEnv = (CEGUI::RadioButton*)pWnd->getChild(pWnd->getName() + "/RBNEnv");
	AddWatched(pRBNEnv->isSelected() ? DEM : Lua, pName);

	OK;
}
//---------------------------------------------------------------------

bool CWatcherWindow::OnListKeyDown(const CEGUI::EventArgs& e)
{
	const CEGUI::KeyEventArgs& ke = (const CEGUI::KeyEventArgs&)e;

	if (ke.scancode == CEGUI::Key::Delete)
	{
		CEGUI::ListboxItem* pSel = pList->getFirstSelectedItem();
		if (pSel)
		{
			CEGUI::uint RowIdx = pList->getRowWithID((int)pSel);
			pList->removeRow(RowIdx);
			
			if (pList->getRowCount())
			{
				if (RowIdx >= pList->getRowCount()) --RowIdx;
				pList->setItemSelectState(CEGUI::MCLGridRef(RowIdx, COL_NAME), true);
			}
			
			for (CArray<CWatched>::CIterator It = Watched.Begin(); It != Watched.End(); It++)
				if (pSel == It->pNameItem)
				{
					It->Clear();
					Watched.Remove(It);
					break;
				}
			
			OK;
		}
	}

	FAIL;
}
//---------------------------------------------------------------------

bool CWatcherWindow::OnClearClick(const CEGUI::EventArgs& e)
{
	pList->resetList();
	for (int i = 0; i < Watched.GetCount(); ++i)
		Watched[i].Clear();
	Watched.Clear();
	OK;
}
//---------------------------------------------------------------------

bool CWatcherWindow::OnAddVarsClick(const CEGUI::EventArgs& e)
{
	AddAllGlobals();
	OK;
}
//---------------------------------------------------------------------

bool CWatcherWindow::OnUIUpdate(const Events::CEventBase& Event)
{
	const char* pPattern = pPatternEdit->getText().c_str();
	bool CheckMatch = (pPattern && *pPattern && strcmp(pPattern, "*"));

	int i = 0;
	for (CArray<CWatched>::CIterator It = Watched.Begin(); It != Watched.End(); It++, ++i)
	{
		if (!CheckMatch || n_strmatch(It->VarName, pPattern))
		{
			if (It->Type == DEM)
			{
				CData CurrVar;

				if (CoreSrv->GetGlobal(It->VarName.CStr(), CurrVar))
				{
					if (CurrVar.IsA<bool>())
					{
						It->pTypeItem->setText("DEM bool");
						It->pValueItem->setText(nString::FromBool(CurrVar.GetValue<bool>()).CStr());
					}
					else if (CurrVar.IsA<int>())
					{
						It->pTypeItem->setText("DEM int");
						It->pValueItem->setText(nString::FromInt(CurrVar.GetValue<int>()).CStr());
					}
					else if (CurrVar.IsA<float>())
					{
						It->pTypeItem->setText("DEM float");
						It->pValueItem->setText(nString::FromFloat(CurrVar.GetValue<float>()).CStr());
					}
					else if (CurrVar.IsA<nString>())
					{
						It->pTypeItem->setText("DEM string");
						It->pValueItem->setText((CEGUI::utf8*)CurrVar.GetValue<nString>().CStr());
					}
					else if (CurrVar.IsA<vector4>())
					{
						It->pTypeItem->setText("DEM vector4");
						It->pValueItem->setText(nString::FromVector4(CurrVar.GetValue<vector4>()).CStr());
					}
					else
					{
						It->pTypeItem->setText("DEM <unknown>");
						It->pValueItem->setText("<unknown>");
					}
					//}
				}
				else
				{
					It->pTypeItem->setText("DEM");
					It->pValueItem->setText("<not found in /sys/var>");
				}
			}
			else if (It->Type == Lua)
			{
				char Script[256];	
				snprintf(Script, sizeof(Script) - 1, "return %s", It->VarName);
				CData Output;
				ScriptSrv->RunScript(Script, -1, &Output);

				//!!!Data.ToString!
				if (Output.IsVoid())
				{
					It->pTypeItem->setText("Lua <unknown>");
					It->pValueItem->setText("nil, not found or unsupported lua type (function, userdata)");
				}
				else if (Output.IsA<bool>())
				{
					It->pTypeItem->setText("Lua bool");
					It->pValueItem->setText((Output == true) ? "true" : "false");
				}
				else if (Output.IsA<int>())
				{
					It->pTypeItem->setText("Lua int");
					It->pValueItem->setText(nString::FromInt(Output).CStr());
				}
				else if (Output.IsA<float>())
				{
					It->pTypeItem->setText("Lua float");
					It->pValueItem->setText(nString::FromFloat(Output).CStr());
				}
				else if (Output.IsA<nString>())
				{
					It->pTypeItem->setText("Lua string");
					It->pValueItem->setText((CEGUI::utf8*)Output.GetValue<nString>().CStr());
				}
				else if (Output.IsA<vector4>())
				{
					It->pTypeItem->setText("Lua vector4");
					It->pValueItem->setText(nString::FromVector4(Output).CStr());
				}
				else if (Output.IsA<PVOID>())
				{
					It->pTypeItem->setText("Lua userdata");
					It->pValueItem->setText(nString::FromInt((int)Output.GetValue<PVOID>()).CStr());
				}
				else if (Output.IsA<PDataArray>())
				{
					It->pTypeItem->setText("Lua array");
					It->pValueItem->setText("[...]");
				}
				else if (Output.IsA<PParams>())
				{
					It->pTypeItem->setText("Lua table");
					It->pValueItem->setText("{...}");
				}
				else
				{
					It->pTypeItem->setText("Lua <unsupported>");
					It->pValueItem->setText("unsupported CData type");
				}
			}

			if (It->RowID == -1)
			{
				It->RowID = (int)It->pNameItem; //???!!!
				int RowIdx = pList->addRow(It->RowID);
				pList->setItem(It->pNameItem, COL_NAME, RowIdx);
				pList->setItem(It->pTypeItem, COL_TYPE, RowIdx);
				pList->setItem(It->pValueItem, COL_VALUE, RowIdx);
			}
		}
		else
		{
			if (It->RowID != -1) pList->removeRow(pList->getRowWithID(It->RowID));
			It->RowID = -1;
		}
	}

	//???!!!how to redraw changed items another way?
	pList->invalidate();

	//if (!MatchCount) textView->AppendLine("no matches");

	OK;
}
//---------------------------------------------------------------------

void CWatcherWindow::CWatched::Clear()
{
	n_delete(pNameItem);
	pNameItem = NULL;
	n_delete(pTypeItem);
	pTypeItem = NULL;
	n_delete(pValueItem);
	pValueItem = NULL;
}
//---------------------------------------------------------------------

}
