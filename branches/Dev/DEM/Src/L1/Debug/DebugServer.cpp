#include "DebugServer.h"

#include <UI/UIServer.h>
#include <Input/InputServer.h>
#include <Events/EventServer.h>
#include <Core/Factory.h>

// Dbg menu plugins:
// console - OK
// texture browser - can create (CC)
// gfx resources - not usable (NU)
// hardpoints - NU
// scene control - NU
// watcher - OK
// system info - CC
// adjust display - CC
// character control - NU
// settings mgmt - NU
// close menu - CC
// quit - OK

namespace Debug
{
__ImplementClassNoFactory(Debug::CDebugServer, Core::CObject);
__ImplementSingleton(Debug::CDebugServer);

CDebugServer::CDebugServer(): UIAllowed(false)
{
	__ConstructSingleton;
	SUBSCRIBE_PEVENT(DebugBreak, CDebugServer, OnDebugBreak);
}
//---------------------------------------------------------------------

CDebugServer::~CDebugServer()
{
	UNSUBSCRIBE_EVENT(DebugBreak);
	__DestructSingleton;
}
//---------------------------------------------------------------------

bool CDebugServer::RegisterPlugin(CStrID Name, LPCSTR CppClassName, LPCSTR UIResource)
{
	CPlugin New;
	New.UIResource = UIResource;
	New.Window = (UI::CUIWindow*)Factory->Create(CString(CppClassName));
	n_assert(New.Window.IsValidPtr());
	//!!!call InitPlugin or write all Init code in the virtual CUIWindow::Init!
	Plugins.Add(Name, New);
	OK;
}
//---------------------------------------------------------------------

//???instead of this subscribe always and inside check UIServer::HasInstance()?
//UI server must destruct plugin windows correctly on its destruction!
void CDebugServer::AllowUI(bool Allow)
{
	UIAllowed = Allow;

	if (UIAllowed)
	{
		SUBSCRIBE_PEVENT(ShowDebugConsole, CDebugServer, OnShowDebugConsole);
		SUBSCRIBE_PEVENT(ShowDebugWatcher, CDebugServer, OnShowDebugWatcher);
	}
	else
	{
		UNSUBSCRIBE_EVENT(ShowDebugConsole);
		UNSUBSCRIBE_EVENT(ShowDebugWatcher);

		for (int i = 0; i < Plugins.GetCount(); ++i)
			if (Plugins.ValueAt(i).UIResource.IsValid())
				Plugins.ValueAt(i).Window->Term();
	}
}
//---------------------------------------------------------------------

bool CDebugServer::OnDebugBreak(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
#ifdef _DEBUG
	__debugbreak();
#endif
	OK;
}
//---------------------------------------------------------------------

bool CDebugServer::OnShowDebugConsole(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	TogglePluginWindow(CStrID("Console"));
	OK;
}
//---------------------------------------------------------------------

bool CDebugServer::OnShowDebugWatcher(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	TogglePluginWindow(CStrID("Watcher"));
	OK;
}
//---------------------------------------------------------------------

void CDebugServer::TogglePluginWindow(CStrID Name)
{
	CPlugin* pPlugin = Plugins.Get(Name);
	if (!pPlugin) return;

	UI::CUIWindow& UI = *pPlugin->Window;
	if (UI.GetWnd() && UI.GetWnd()->getParent() == UISrv->GetRootScreen()->GetWnd())
		UI.ToggleVisibility();
	else
	{
		if (!UI.GetWnd()) 
		{
			UI.Load(CString(pPlugin->UIResource.CStr()));
			n_assert(UI.GetWnd());
		}
		if (UI.GetWnd()->getParent() != UISrv->GetRootScreen()->GetWnd())
		{
			UISrv->GetRootScreen()->GetWnd()->addChild(UI.GetWnd());
			UI.Hide(); // To force OnShow event
			UI.Show();
		}
	}
	//UI.SetInputFocus();
}
//---------------------------------------------------------------------

void CDebugServer::Trigger()
{
}
//---------------------------------------------------------------------

} //namespace Debug