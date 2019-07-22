// CEGUI uses insecure function in a template class -_-
#pragma warning(push)
#pragma warning(disable : 4996)       // _CRT_INSECURE_DEPRECATE, VS8: old string routines are deprecated

#include "WatcherWindow.h"

#include <Scripting/ScriptServer.h>
#include <Events/EventServer.h>
#include <Data/DataArray.h>
#include <Data/StringUtils.h>
#include <Core/CoreServer.h>
#include <Core/Factory.h>

#include <CEGUI/widgets/Editbox.h>
#include <CEGUI/widgets/MultiColumnList.h>
#include <CEGUI/widgets/ListboxTextItem.h>
#include <CEGUI/widgets/RadioButton.h>
#include <CEGUI/widgets/PushButton.h>

#define COL_NAME	0
#define COL_TYPE	1
#define COL_VALUE	2

namespace Debug
{
__ImplementClass(Debug::CWatcherWindow, 'DWWW', UI::CUIWindow);

CWatcherWindow::CWatcherWindow()
{
	Init();
}
//---------------------------------------------------------------------

CWatcherWindow::~CWatcherWindow()
{
	for (UPTR i = 0; i < Watched.GetCount(); ++i)
		Watched[i].Clear();

	UNSUBSCRIBE_EVENT(OnUIUpdate);
}
//---------------------------------------------------------------------

void CWatcherWindow::Init()
{
	pWnd->setDrawModeMask(UI::DrawModeFlagWindowOpaque);
	UPTR CEGUIChildCount = pWnd->getChildCount();
	for (UPTR i = 0; i < CEGUIChildCount; ++i)
		pWnd->getChildAtIdx(i)->setDrawModeMask(UI::DrawModeFlagWindowOpaque);

	pPatternEdit = (CEGUI::Editbox*)pWnd->getChild("PatternEdit");

	pList = (CEGUI::MultiColumnList*)pWnd->getChild("List");
	pList->addColumn("Name", COL_NAME, CEGUI::UDim(0, 300));
	pList->addColumn("Type", COL_TYPE, CEGUI::UDim(0, 100));
	pList->addColumn("Value", COL_VALUE, CEGUI::UDim(0, 200));
	pList->setSelectionMode(CEGUI::MultiColumnList::SelectionMode::RowSingle);
	pList->subscribeEvent(CEGUI::MultiColumnList::EventSemanticEvent,
		CEGUI::Event::Subscriber(&CWatcherWindow::OnListSemanticEvent, this));

	CEGUI::RadioButton* pRBNEnv = (CEGUI::RadioButton*)pWnd->getChild("RBNEnv");
	pRBNEnv->setGroupID(0);

	CEGUI::RadioButton* pRBLua = (CEGUI::RadioButton*)pWnd->getChild("RBLua");
	pRBLua->setGroupID(0);

	pRBNEnv->setSelected(true);

	pNewWatchEdit = (CEGUI::Editbox*)pWnd->getChild("NewWatchEdit");
	pNewWatchEdit->subscribeEvent(CEGUI::Editbox::EventTextAccepted,
		CEGUI::Event::Subscriber(&CWatcherWindow::OnNewWatchedAccept, this));

	pWnd->getChild("BtnClear")->subscribeEvent(CEGUI::PushButton::EventClicked,
		CEGUI::Event::Subscriber(&CWatcherWindow::OnClearClick, this));

	pWnd->getChild("BtnAllVars")->subscribeEvent(CEGUI::PushButton::EventClicked,
		CEGUI::Event::Subscriber(&CWatcherWindow::OnAddVarsClick, this));

	if (IsVisible()) SUBSCRIBE_PEVENT(OnUIUpdate, CWatcherWindow, OnUIUpdate);
}
//---------------------------------------------------------------------

void CWatcherWindow::SetVisible(bool Visible)
{
	if (Visible) SUBSCRIBE_PEVENT(OnUIUpdate, CWatcherWindow, OnUIUpdate);
	else UNSUBSCRIBE_EVENT(OnUIUpdate);
	CUIWindow::SetVisible(Visible);
}
//---------------------------------------------------------------------

void CWatcherWindow::AddWatched(EVarType Type, const char* Name)
{
	CWatched& Curr = *Watched.Reserve(1);
	Curr.Type = Type;
	Curr.VarName = Name;

	if (!Curr.pNameItem)
	{
		Curr.pNameItem = n_new(CEGUI::ListboxTextItem(Name, 0, 0, false, false));
		Curr.pNameItem->setSelectionBrushImage("TaharezLook/MultiListSelectionBrush");
		Curr.pNameItem->setSelectionColours(CEGUI::Colour(0xff606099));
		Curr.pNameItem->setTextParsingEnabled(false);
	}
	else Curr.pNameItem->setText(Name);

	if (!Curr.pTypeItem)
	{
		Curr.pTypeItem = n_new(CEGUI::ListboxTextItem("", 0, 0, false, false));
		Curr.pTypeItem->setSelectionBrushImage("TaharezLook/MultiListSelectionBrush");
		Curr.pTypeItem->setSelectionColours(CEGUI::Colour(0xff606099));
		Curr.pTypeItem->setTextParsingEnabled(false);
	}

	if (!Curr.pValueItem)
	{
		Curr.pValueItem = n_new(CEGUI::ListboxTextItem("", 0, 0, false, false));
		Curr.pValueItem->setSelectionBrushImage("TaharezLook/MultiListSelectionBrush");
		Curr.pValueItem->setSelectionColours(CEGUI::Colour(0xff606099));
		Curr.pValueItem->setTextParsingEnabled(false);
	}
}
//---------------------------------------------------------------------

void CWatcherWindow::AddAllGlobals()
{
	UPTR i = Watched.GetCount();
	for (CHashTable<CString, Data::CData>::CIterator It = CoreSrv->Globals.Begin(); It; ++It, ++i)
		AddWatched(DEM, It.GetKey().CStr());

	for (UPTR j = i; j < Watched.GetCount(); ++j)
		Watched[j].Clear();
	Watched.Resize(i);
}
//---------------------------------------------------------------------

bool CWatcherWindow::OnNewWatchedAccept(const CEGUI::EventArgs& e)
{
	std::string Name = CEGUI::String::convertUtf32ToUtf8(pNewWatchEdit->getText().c_str());
	if (Name.empty()) OK;

	CEGUI::RadioButton* pRBNEnv = (CEGUI::RadioButton*)pWnd->getChild("RBNEnv");
	AddWatched(pRBNEnv->isSelected() ? DEM : Lua, Name.c_str());

	OK;
}
//---------------------------------------------------------------------

bool CWatcherWindow::OnListSemanticEvent(const CEGUI::EventArgs& e)
{
	const CEGUI::SemanticEventArgs& ke = static_cast<const CEGUI::SemanticEventArgs&>(e);

	if (ke.d_semanticValue == CEGUI::SemanticValue::DeleteNextCharacter)
	{
		CEGUI::ListboxItem* pSel = pList->getFirstSelectedItem();
		if (pSel)
		{
			unsigned int RowIdx = pList->getRowWithID((int)pSel);
			pList->removeRow(RowIdx);
			
			if (pList->getRowCount())
			{
				if (RowIdx >= pList->getRowCount()) --RowIdx;
				pList->setItemSelectState(CEGUI::MCLGridRef(RowIdx, COL_NAME), true);
			}
			
			for (CArray<CWatched>::CIterator It = Watched.Begin(); It != Watched.End(); ++It)
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
	for (UPTR i = 0; i < Watched.GetCount(); ++i)
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

bool CWatcherWindow::OnUIUpdate(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	std::string Pattern = CEGUI::String::convertUtf32ToUtf8(pPatternEdit->getText().c_str());
	const bool CheckMatch = !Pattern.empty() && Pattern != "*";

	int i = 0;
	for (CArray<CWatched>::CIterator It = Watched.Begin(); It != Watched.End(); It++, ++i)
	{
		if (!CheckMatch || StringUtils::MatchesPattern(It->VarName, Pattern.c_str()))
		{
			if (It->Type == DEM)
			{
				Data::CData CurrVar;

				if (CoreSrv->GetGlobal(CString(It->VarName.CStr()), CurrVar))
				{
					if (CurrVar.IsA<bool>())
					{
						It->pTypeItem->setText("DEM bool");
						It->pValueItem->setText(StringUtils::FromBool(CurrVar.GetValue<bool>()).CStr());
					}
					else if (CurrVar.IsA<int>())
					{
						It->pTypeItem->setText("DEM int");
						It->pValueItem->setText(StringUtils::FromInt(CurrVar.GetValue<int>()).CStr());
					}
					else if (CurrVar.IsA<float>())
					{
						It->pTypeItem->setText("DEM float");
						It->pValueItem->setText(StringUtils::FromFloat(CurrVar.GetValue<float>()).CStr());
					}
					else if (CurrVar.IsA<CString>())
					{
						It->pTypeItem->setText("DEM string");
						It->pValueItem->setText(CurrVar.GetValue<CString>().CStr());
					}
					else if (CurrVar.IsA<vector4>())
					{
						It->pTypeItem->setText("DEM vector4");
						It->pValueItem->setText(StringUtils::FromVector4(CurrVar.GetValue<vector4>()).CStr());
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
				_snprintf(Script, sizeof(Script) - 1, "return %s", It->VarName.CStr());
				Data::CData Output;
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
					It->pValueItem->setText(StringUtils::FromInt(Output).CStr());
				}
				else if (Output.IsA<float>())
				{
					It->pTypeItem->setText("Lua float");
					It->pValueItem->setText(StringUtils::FromFloat(Output).CStr());
				}
				else if (Output.IsA<CString>())
				{
					It->pTypeItem->setText("Lua string");
					It->pValueItem->setText(Output.GetValue<CString>().CStr());
				}
				else if (Output.IsA<vector4>())
				{
					It->pTypeItem->setText("Lua vector4");
					It->pValueItem->setText(StringUtils::FromVector4(Output).CStr());
				}
				else if (Output.IsA<PVOID>())
				{
					It->pTypeItem->setText("Lua userdata");
					It->pValueItem->setText(StringUtils::FromInt((int)Output.GetValue<PVOID>()).CStr());
				}
				else if (Output.IsA<Data::PDataArray>())
				{
					It->pTypeItem->setText("Lua array");
					It->pValueItem->setText("[...]");
				}
				else if (Output.IsA<Data::PParams>())
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

#pragma warning(pop)
