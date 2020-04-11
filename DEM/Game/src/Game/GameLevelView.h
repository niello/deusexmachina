#pragma once
#include <Data/Ptr.h>
#include <Game/ECS/Entity.h>
#include <Math/Vector3.h>
#include <optional>

// Represents a client view of a game location. Manages rendering and input.
// In MVC pattern it is a view.

//???camera controller here?

namespace Scene
{
	class CSceneNode;
}

namespace Frame
{
	class CView;
}

namespace DEM::Game
{
typedef Ptr<class CGameLevel> PGameLevel;
typedef std::unique_ptr<class CGameLevelView> PGameLevelView;

class CGameLevelView final
{
protected:

	PGameLevel             _Level;
	Frame::CView&          _View; //???or hold strong/weak reference?

	HEntity                _EntityUnderCursor;
	Scene::CSceneNode*     _pNodeUnderCursor = nullptr;
	std::optional<vector3> _PointUnderCursor;

	//???different debug draw options? Physics, separate property types etc?

public:

	CGameLevelView(CGameLevel& Level, Frame::CView& View);

	void Update(float dt);

	CGameLevel*        GetLevel() const { return _Level.Get(); }
	Frame::CView&      GetFrameView() const { return _View; }

	bool               HasWorldCursorIntersection() const { return _PointUnderCursor.has_value(); }
	HEntity            GetEntityUnderCursor() const { return _EntityUnderCursor; }
	Scene::CSceneNode* GetSceneNodeUnderCursor() const { return _pNodeUnderCursor; }
	const vector3&     GetPointUnderCursor() const { return _PointUnderCursor.has_value() ? _PointUnderCursor.value() : vector3::Zero; }
};

}


//////////////// TODO: REMOVE ///////////////////////////////
#include <Frame/View.h>
#include <Data/StringID.h>
#include <Math/Vector3.h>
#include <Events/EventsFwd.h>

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
	Frame::PView		View;
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
	Frame::CView&			GetFrameView() { return *View; }
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
