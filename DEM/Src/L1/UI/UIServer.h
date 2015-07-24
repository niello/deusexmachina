#pragma once
#ifndef __DEM_L1_UI_SERVER_H__
#define __DEM_L1_UI_SERVER_H__

#include <UI/UIWindow.h>
#include <Data/Singleton.h>
#include <Data/StringID.h>
#include <Events/EventsFwd.h>
#include <Data/Dictionary.h>
#include <CEGUI/Event.h>
#include <CEGUI/Vector.h>

// UI server (CEGUI launcher & manager). This server creates top-level screens and manages
// their switching (or provides switching functionality to application states).

//???May be should control global state of UI (all tips above IAOs are visible or not etc). Should load windows.

namespace CEGUI
{
	class CDEMLogger;
	class CDEMRenderer;
	class CDEMResourceProvider;
	class TinyXML2Parser;
}

namespace UI
{
typedef Ptr<class CUIWindow> PUIWindow;

#define UISrv UI::CUIServer::Instance()

class CUIServer: public Core::CObject
{
	__DeclareClassNoFactory;
	__DeclareSingleton(CUIServer);

private:

	CEGUI::CDEMRenderer*				Renderer;
	CEGUI::System*						CEGUISystem;
	CEGUI::CDEMLogger*					Logger;
	CEGUI::CDEMResourceProvider*		ResourceProvider;
	CEGUI::TinyXML2Parser*				XMLParser;

	CDict<CStrID, PUIWindow>			Screens;
	CUIWindow*							CurrRootScreen;

	CArray<CEGUI::Event::Connection>	ConnectionsToDisconnect;

	DECLARE_EVENT_HANDLER(KeyDown, OnKeyDown);
	DECLARE_EVENT_HANDLER(KeyUp, OnKeyUp);
	DECLARE_EVENT_HANDLER(CharInput, OnCharInput);
	DECLARE_EVENT_HANDLER(MouseMove, OnMouseMove);
	DECLARE_EVENT_HANDLER(MouseBtnDown, OnMouseBtnDown);
	DECLARE_EVENT_HANDLER(MouseBtnUp, OnMouseBtnUp);
	DECLARE_EVENT_HANDLER(MouseWheel, OnMouseWheel);
	DECLARE_EVENT_HANDLER(OnRenderDeviceLost, OnDeviceLost);
	DECLARE_EVENT_HANDLER(OnRenderDeviceReset, OnDeviceReset);

public:

	CUIServer();
	~CUIServer();
	
	// Internal use, set by config
	void			LoadScheme(const char* pResourceFile);
	void			LoadFont(const char* pResourceFile); //???retval CEGUI::Font&?
	void			SetDefaultMouseCursor(const char* pImageName);
	//!!!create dynamic fonts! see article!

	//bool Init(PParams Cfg);
	void Trigger(float FrameTime);
	void Render();

	// Interface
	bool			RegisterScreen(CStrID Name, CUIWindow* pScreen);
	//Ptr<CUIWindow>	LoadScreen(CStrID Name, const char* pResourceFile);
	CUIWindow*		GetScreen(CStrID Name) const;
	void			SetRootScreen(CUIWindow* pWindow);
	CUIWindow*		GetRootScreen() const { return CurrRootScreen; }
	void			SetRootWindow(CEGUI::Window* pWindow);
	bool			SetRootWindow(CStrID Name);
	//CUIWindow*
	CEGUI::Window*	GetRootWindow() const;
	void			DestroyWindow(CUIWindow* pWindow);
	void			DestroyWindow(CStrID Name);

	void			ShowGUI();
	void			HideGUI();
	bool			IsGUIVisible() const;
	bool			IsMouseOverGUI() const;

	void			ShowMouseCursor();
	void			HideMouseCursor();

	// Internal use by UI system & windows
	CEGUI::UVector2	GetMousePositionU() const;
	CEGUI::Vector2f	GetMousePosition() const; //???return nebula vector2, not point? vector2 can be used externally
	
	// Event will be disconnected at the end of GUI render loop.
	// Attention! This method is not thread safe. You must call
	// it only from GUI thread.
	void			DelayedDisconnect(CEGUI::Event::Connection Connection) { ConnectionsToDisconnect.Add(Connection); }
	/*
	void UnloadScheme(const char* schemeName);
	void UnloadAllSchemes();
	void CreateImageSet(const char* imagesetName, const char* fileName);
	void DestroyImageSet(const char* imagesetName);
	void DestroyAllImageSets();
	void UnloadFont(const char* FontName);
	void DestroyAllFonts();
	void DestroyAllWindows();
	*/
};

inline CUIWindow* CUIServer::GetScreen(CStrID Name) const
{
	int Idx = Screens.FindIndex(Name);
	return Idx != INVALID_INDEX ? Screens.ValueAt(Idx) : NULL;
}
//---------------------------------------------------------------------

}

#endif
