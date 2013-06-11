#pragma once
#ifndef __DEM_L1_UI_SERVER_H__
#define __DEM_L1_UI_SERVER_H__

#include <UI/Window.h>
#include <Core/Singleton.h>
#include <Data/StringID.h>
#include <Events/Events.h>
#include <util/ndictionary.h>
#include <CEGUIEvent.h>
#include <CEGUIVector.h>

// UI server (CEGUI launcher & manager). This server creates top-level screens and manages
// their switching (or provides switching functionality to application states).

//???May be should control global state of UI (all tips
//above IAOs are visible or not etc). Should load windows.

namespace CEGUI
{
	class Direct3D9Renderer;
	class CNebula2Logger;
	class CNebula2ResourceProvider;
	class TinyXML2Parser;
	class System;
	class Font;
	class Window;
}

namespace UI
{
typedef Ptr<class CWindow> PWindow;

#define UISrv UI::CUIServer::Instance()

class CUIServer: public Core::CRefCounted
{
	__DeclareClassNoFactory;
	__DeclareSingleton(CUIServer);

private:

	CEGUI::Direct3D9Renderer*			Renderer; //!!!use N2 renderer!
	CEGUI::TinyXML2Parser*				Parser;
	CEGUI::System*						CEGUISystem;
	CEGUI::CNebula2Logger*				Logger;
	CEGUI::CNebula2ResourceProvider*	ResourceProvider;

	nDictionary<CStrID, PWindow>		Screens;
	CWindow*							CurrRootScreen;

	nArray<CEGUI::Event::Connection>	ConnectionsToDisconnect;

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
	void			LoadScheme(const nString& ResourceFile);
	void			LoadFont(const nString& ResourceFile); //???retval CEGUI::Font&?
	void			SetDefaultMouseCursor(const nString& SchemeName, const nString& CursorName);
	//!!!create dynamic fonts! see article!

	//bool Init(PParams Cfg);
	void Trigger(float FrameTime);
	void Render();

	// Interface
	bool			RegisterScreen(CStrID Name, CWindow* pScreen);
	//Ptr<CWindow>	LoadScreen(CStrID Name, const nString& ResourceFile);
	CWindow*		GetScreen(CStrID Name) const;
	void			SetRootScreen(CWindow* pWindow);
	CWindow*		GetRootScreen() const { return CurrRootScreen; }
	void			SetRootWindow(CEGUI::Window* pWindow);
	bool			SetRootWindow(CStrID Name);
	//CWindow*
	CEGUI::Window*	GetRootWindow() const;
	void			DestroyWindow(CWindow* pWindow);
	void			DestroyWindow(CStrID Name);

	void			ShowGUI();
	void			HideGUI();
	bool			IsGUIVisible() const;
	bool			IsMouseOverGUI() const;

	// Internal use by UI system & windows
	CEGUI::UVector2	GetMousePositionU() const;
	CEGUI::Point	GetMousePosition() const; //???return nebula vector2, not point? vector2 can be used externally
	
	// Event will be disconnected at the end of GUI render loop.
	// Attention! This method is not thread safe. You must call
	// it only from GUI thread.
	void			DelayedDisconnect(CEGUI::Event::Connection Connection) { ConnectionsToDisconnect.Append(Connection); }
	/*
	void UnloadScheme(const nString& schemeName);
	void UnloadAllSchemes();
	void CreateImageSet(const nString& imagesetName, const nString& fileName);
	void DestroyImageSet(const nString& imagesetName);
	void DestroyAllImageSets();
	void UnloadFont(const nString& FontName);
	void DestroyAllFonts();
	void DestroyAllWindows();
	*/
};

inline CWindow* CUIServer::GetScreen(CStrID Name) const
{
	int Idx = Screens.FindIndex(Name);
	return Idx != INVALID_INDEX ? Screens.ValueAt(Idx) : NULL;
}
//---------------------------------------------------------------------

}

#endif
