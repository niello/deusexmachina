#include "GameLevelView.h"
#include <Game/GameLevel.h>
#include <Game/ECS/Entity.h>
#include <Frame/SceneNodePrecreateRenderObjects.h>
#include <Frame/SceneNodeUpdateInSPS.h>
#include <Frame/CameraAttribute.h>
#include <UI/UIContext.h>
#include <Physics/RigidBody.h>
#include <Physics/CollisionAttribute.h>
#include <Physics/PhysicsLevel.h>
#include <Scene/SceneNode.h>
#include <Scene/SceneNodeRenderDebug.h>

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

void CGameLevelView::Update(float dt)
{
	if (_Level)
	{
		if (auto pDebugDraw = _View.GetDebugDrawer())
		{
			_Level->GetSceneRoot().AcceptVisitor(Scene::CSceneNodeRenderDebug(*pDebugDraw));
			_Level->GetPhysics()->RenderDebug(*pDebugDraw);
		}
	}

	HEntity OldEntityUnderCursor = _EntityUnderCursor;

	_PointUnderCursor.reset();
	_pNodeUnderCursor = nullptr;
	_EntityUnderCursor = HEntity{};

	if (_Level && !_View.GetUIContext()->IsMouseOverGUI())
	{
		if (const auto* pCamera = _View.GetCamera())
		{
			float XRel, YRel;
			_View.GetUIContext()->GetCursorPositionRel(XRel, YRel);

			line3 Ray;
			pCamera->GetRay3D(XRel, YRel, pCamera->GetFarPlane(), Ray);

			vector3 Point;
			if (auto pPhysicsObject = _Level->GetFirstPickIntersection(Ray, &Point))
			{
				_PointUnderCursor = Point;

				if (pPhysicsObject->UserData().has_value())
				{
					if (auto pRB = pPhysicsObject->As<Physics::CRigidBody>())
					{
						if (auto pHEntity = std::any_cast<HEntity>(&pPhysicsObject->UserData()))
							_EntityUnderCursor = *pHEntity;
						_pNodeUnderCursor = pRB->GetControlledNode();
					}
					else
					{
						if (auto pPair = std::any_cast<std::pair<HEntity, Physics::CCollisionAttribute*>>(&pPhysicsObject->UserData()))
						{
							_EntityUnderCursor = pPair->first;
							_pNodeUnderCursor = pPair->second->GetNode();
						}
					}
				}
			}
		}
	}

	if (OldEntityUnderCursor != _EntityUnderCursor)
	{
		//std::string OldStr = OldEntityUnderCursor ? std::to_string(OldEntityUnderCursor) : "<none>";
		//std::string NewStr = _EntityUnderCursor ? std::to_string(_EntityUnderCursor) : "<none>";
		//::Sys::DbgOut((OldStr + " -> " + NewStr + '\n').c_str());

		//Data::PParams P = n_new(Data::CParams(1));
		//P->Set<PVOID>(CStrID("LevelViewPtr"), this);
		//Game::CEntity* pEntityUnderMouse = GameSrv->GetEntityMgr()->GetEntity(OldEntityUnderMouse);
		//if (pEntityUnderMouse) pEntityUnderMouse->FireEvent(CStrID("OnMouseLeave"), P);
		//pEntityUnderMouse = GameSrv->GetEntityMgr()->GetEntity(EntityUnderMouse);
		//if (pEntityUnderMouse) pEntityUnderMouse->FireEvent(CStrID("OnMouseEnter"), P);
	}
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

	if (!Level || View->GetUIContext()->IsMouseOverGUI()) HasMouseIsect = false;
	else
	{
		float XRel, YRel;
		View->GetUIContext()->GetCursorPositionRel(XRel, YRel);

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