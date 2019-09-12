#pragma once
#ifndef __DEM_L2_GAME_LEVEL_VIEW_H__
#define __DEM_L2_GAME_LEVEL_VIEW_H__

#include <Frame/View.h>
#include <Data/StringID.h>
#include <Math/Vector3.h>
#include <Events/EventsFwd.h>

// Represents a client view of a game location. Manages rendering and input.
// In MVC pattern level view would be a View.

namespace Data
{
	struct CRectF;
}

namespace Game
{
typedef Ptr<class CGameLevel> PGameLevel;
class CEntity;

class CGameLevelView
{
protected:

	HHandle				Handle;
	PGameLevel			Level;
	Frame::CView		View;
	//???store game camera here or obtain from manager? load camera in view loading!

	CArray<CStrID>		SelectedEntities;

	CStrID				EntityUnderMouse;
	bool				HasMouseIsect;
	vector3				MousePos3D;

	DECLARE_EVENT_HANDLER(OnEntityDeactivated, OnEntityDeactivated);

public:

	CGameLevelView();
	~CGameLevelView();

	bool					Setup(CGameLevel& GameLevel, HHandle hView);
	void					Trigger();

	HHandle					GetHandle() const { return Handle; }
	CGameLevel*				GetLevel() const { return Level.Get(); }
	Frame::CView&			GetFrameView() { return View; }
	const vector3&			GetCenterOfInterest() const;

	void					AddToSelection(CStrID EntityID);
	bool					RemoveFromSelection(CStrID EntityID);
	void					ClearSelection() { SelectedEntities.Clear(); }
	const CArray<CStrID>&	GetSelection() const { return SelectedEntities; }
	UPTR					GetSelectedCount() const { return SelectedEntities.GetCount(); }
	bool					IsSelected(CStrID EntityID) const { return SelectedEntities.Contains(EntityID); }

	bool					GetEntityScreenRectRel(Data::CRectF& Out, const Game::CEntity& Entity, const vector3* Offset = nullptr) const;

	bool					HasMouseIntersection(/*HHandle hView*/) const { return HasMouseIsect; }
	CStrID					GetEntityUnderMouseUID(/*HHandle hView*/) const { return EntityUnderMouse; }
	const vector3&			GetMousePos3D(/*HHandle hView*/) const { return MousePos3D; }
};

}

#endif
