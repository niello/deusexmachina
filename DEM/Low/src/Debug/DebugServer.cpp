#include "DebugServer.h"

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
__ImplementSingleton(Debug::CDebugServer);

CDebugServer::CDebugServer()
{
	__ConstructSingleton;
}
//---------------------------------------------------------------------

CDebugServer::~CDebugServer()
{
	__DestructSingleton;
}
//---------------------------------------------------------------------

bool CDebugServer::RegisterPlugin(CStrID Name, const char* CppClassName, const char* UIResource)
{
	CPlugin New;
	New.UIResource = UIResource;
	New.Window = static_cast<UI::CUIWindow*>(DEM::Core::CFactory::Instance().Create(CppClassName));
	n_assert(New.Window.IsValidPtr());
	//!!!call InitPlugin or write all Init code in the virtual CUIWindow::Init!
	Plugins.Add(Name, New);
	OK;
}
//---------------------------------------------------------------------

//???instead of this subscribe always and inside check UIServer::HasInstance()?
//UI server must destruct plugin windows correctly on its destruction!
void CDebugServer::SetUIContext(UI::PUIContext Context)
{
	if (UIContext == Context) return;

	UIContext = Context;

	// TODO: if context changed, transfer active debug windows to it

	if (UIContext)
	{
		//SUBSCRIBE_PEVENT(ShowDebugConsole, CDebugServer, OnShowDebugConsole);
		//SUBSCRIBE_PEVENT(ShowDebugWatcher, CDebugServer, OnShowDebugWatcher);
	}
	else
	{
		//UNSUBSCRIBE_EVENT(ShowDebugConsole);
		//UNSUBSCRIBE_EVENT(ShowDebugWatcher);

		for (UPTR i = 0; i < Plugins.GetCount(); ++i)
			if (!Plugins.ValueAt(i).UIResource.empty())
				Plugins.ValueAt(i).Window.Reset();
	}
}
//---------------------------------------------------------------------

void CDebugServer::TogglePluginWindow(CStrID Name)
{
	CPlugin* pPlugin = Plugins.Get(Name);
	if (!pPlugin) return;

	UI::CUIWindow& UI = *pPlugin->Window;
	UI::CUIWindow& RootWnd = *UIContext->GetRootWindow();

	if (UI.GetCEGUIWindow() && UI.GetCEGUIWindow()->getParent() == RootWnd.GetCEGUIWindow())
		UI.ToggleVisibility();
	else
	{
		if (!UI.GetCEGUIWindow())
		{
			UI.Load(pPlugin->UIResource.c_str());
			n_assert(UI.GetCEGUIWindow());
		}
		if (UI.GetCEGUIWindow()->getParent() != RootWnd.GetCEGUIWindow())
		{
			RootWnd.GetCEGUIWindow()->addChild(UI.GetCEGUIWindow());
			UI.Hide(); // To force OnShow event
			UI.Show();
		}
	}

	UI.SetFocus();
}
//---------------------------------------------------------------------

void CDebugServer::Trigger()
{
}
//---------------------------------------------------------------------

}
