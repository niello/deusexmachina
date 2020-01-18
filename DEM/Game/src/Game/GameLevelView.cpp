#include "GameLevelView.h"
#include <Game/GameLevel.h>
#include <Frame/SceneNodePrecreateRenderObjects.h>
#include <Frame/SceneNodeUpdateInSPS.h>
#include <Scene/SceneNode.h>

namespace DEM::Game
{

CGameLevelView::CGameLevelView(CGameLevel& Level, Frame::CView& View)
	: _Level(&Level)
	, _View(View)
{
	_View.pSPS = &_Level->GetSPS();
	_Level->GetSceneRoot().AcceptVisitor(Frame::CSceneNodePrecreateRenderObjects(_View));
}
//---------------------------------------------------------------------

}


//////////////// TODO: REMOVE ///////////////////////////////
#include <Game/GameServer.h>
#include <Game/Entity.h>
#include <Scene/PropSceneNode.h>
#include <Frame/CameraAttribute.h>
#include <Render/RenderTarget.h>
#include <UI/UIContext.h>
#include <Events/Subscription.h>
#include <Data/Regions.h>

namespace Game
{
CGameLevelView::CGameLevelView() {}
CGameLevelView::~CGameLevelView() {}

bool CGameLevelView::Setup(CGameLevel& GameLevel, HHandle hView)
{
	Level = &GameLevel;
	View->pSPS = &GameLevel.GetSPS();
	//???fill other fields?
	DISP_SUBSCRIBE_PEVENT(Level.Get(), OnEntityDeactivated, CGameLevelView, OnEntityDeactivated);
	OK;
}
//---------------------------------------------------------------------

void CGameLevelView::Trigger()
{
	CStrID OldEntityUnderMouse = EntityUnderMouse;

	if (Level.IsNullPtr() || View->UIContext->IsMouseOverGUI()) HasMouseIsect = false;
	else
	{
		float XRel, YRel;
		View->UIContext->GetCursorPositionRel(XRel, YRel);

		const Frame::CCameraAttribute* pCamera = View->GetCamera();
		if (pCamera)
		{
			line3 Ray;
			pCamera->GetRay3D(XRel, YRel, 5000.f, Ray); //???ray length to far plane or infinite?
			HasMouseIsect = Level->GetFirstIntersectedEntity(Ray, &MousePos3D, &EntityUnderMouse);
		}
		else HasMouseIsect = false;
	}

	if (!HasMouseIsect)
	{
		EntityUnderMouse = CStrID::Empty;
		MousePos3D.set(0.0f, 0.0f, 0.0f);
	}

	if (OldEntityUnderMouse != EntityUnderMouse)
	{
		Data::PParams P = n_new(Data::CParams(1));
		P->Set<PVOID>(CStrID("LevelViewPtr"), this);
		Game::CEntity* pEntityUnderMouse = GameSrv->GetEntityMgr()->GetEntity(OldEntityUnderMouse);
		if (pEntityUnderMouse) pEntityUnderMouse->FireEvent(CStrID("OnMouseLeave"), P);
		pEntityUnderMouse = GameSrv->GetEntityMgr()->GetEntity(EntityUnderMouse);
		if (pEntityUnderMouse) pEntityUnderMouse->FireEvent(CStrID("OnMouseEnter"), P);
	}
}
//---------------------------------------------------------------------

const vector3& CGameLevelView::GetCenterOfInterest() const
{
	return View->GetCamera()->GetNode()->GetWorldPosition();
}
//---------------------------------------------------------------------

void CGameLevelView::AddToSelection(CStrID EntityID)
{
	if (IsSelected(EntityID) || !EntityID.IsValid() || !Level->HostsEntity(EntityID)) return;
	SelectedEntities.Add(EntityID);
	Data::PParams P = n_new(Data::CParams(1));
	P->Set(CStrID("EntityID"), EntityID);
	//???add view handle as int? or as UPTR/IPTR?
	Level->FireEvent(CStrID("OnEntitySelected"), P);
}
//---------------------------------------------------------------------

bool CGameLevelView::RemoveFromSelection(CStrID EntityID)
{
	if (!SelectedEntities.RemoveByValue(EntityID)) FAIL; 
	Data::PParams P = n_new(Data::CParams(1));
	P->Set(CStrID("EntityID"), EntityID);
	Level->FireEvent(CStrID("OnEntityDeselected"), P);
	OK;
}
//---------------------------------------------------------------------

bool CGameLevelView::GetEntityScreenRectRel(Data::CRectF& Out, const Game::CEntity& Entity, const vector3* Offset) const
{
	const Frame::CCameraAttribute* pCamera = View->GetCamera();
	if (!pCamera) FAIL;

	Prop::CPropSceneNode* pNode = Entity.GetProperty<Prop::CPropSceneNode>();
	if (!pNode)
	{
		matrix44 Tfm;
		if (!Entity.GetAttr(Tfm, CStrID("Transform"))) FAIL;
		float X, Y;
		pCamera->GetPoint2D(Tfm.Translation(), X, Y);
		Out.X = X;
		Out.Y = Y;
		Out.W = 0.f;
		Out.H = 0.f;
		OK;
	}

	CAABB AABB;
	pNode->GetAABB(AABB);

	if (Offset)
	{
		AABB.Max += *Offset;
		AABB.Min += *Offset;
	}

	float X, Y;
	pCamera->GetPoint2D(AABB.GetCorner(0), X, Y);

	float Right = X, Top = Y;
	vector2 ScreenPos;
	for (UPTR i = 1; i < 8; ++i)
	{
		pCamera->GetPoint2D(AABB.GetCorner(i), ScreenPos.x, ScreenPos.y);

		if (ScreenPos.x < X) X = ScreenPos.x;
		else if (ScreenPos.x > Right) Right = ScreenPos.x;

		if (ScreenPos.y < Y) Y = ScreenPos.y;
		else if (ScreenPos.y > Top) Top = ScreenPos.y;
	}

	Out.X = X;
	Out.Y = Y;
	Out.W = Right - X;
	Out.H = Top - Y;

	OK;
}
//---------------------------------------------------------------------

bool CGameLevelView::OnEntityDeactivated(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	Data::PParams P = ((const Events::CEvent&)Event).Params;
	SelectedEntities.RemoveByValue(P->Get<CStrID>(CStrID("EntityID"))); 
	OK;
}
//---------------------------------------------------------------------

}