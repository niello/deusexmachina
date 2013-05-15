#include "EnvQueryManager.h"

#include <UI/UIServer.h>
#include <Data/Params.h>
#include <Physics/PhysicsServer.h>
#include <Input/InputServer.h>
#include <Game/EntityManager.h>
#include <Events/EventManager.h>
#include <Scene/PropSceneNode.h>
#include <Scene/SceneServer.h>
#include <Physics/Prop/PropAbstractPhysics.h>

namespace Game
{
__ImplementClassNoFactory(Game::CEnvQueryManager, Game::CManager);
__ImplementClass(Game::CEnvQueryManager);
__ImplementSingleton(CEnvQueryManager);

CEnvQueryManager::CEnvQueryManager():
	EntityUnderMouse(CStrID::Empty),
	MouseIntersection(false)
{
	__ConstructSingleton;
}
//---------------------------------------------------------------------

CEnvQueryManager::~CEnvQueryManager()
{
	__DestructSingleton;
}
//---------------------------------------------------------------------

void CEnvQueryManager::Activate()
{
	CManager::Activate();
	SUBSCRIBE_PEVENT(OnFrame, CEnvQueryManager, OnFrame);
}
//---------------------------------------------------------------------

void CEnvQueryManager::Deactivate()
{
	UNSUBSCRIBE_EVENT(OnFrame);
	CManager::Deactivate();
}
//---------------------------------------------------------------------

CEntity* CEnvQueryManager::GetEntityUnderMouse() const
{
	return EntityMgr->GetEntity(EntityUnderMouse);
}
//---------------------------------------------------------------------

/*
void CEnvQueryManager::GetEntitiesUnderMouseDragDropRect(const rectangle& DragDropRect,
														 nArray<PEntity>& Entities)
{
	line3 Ray = nGfxServer2::Instance()->ComputeWorldMouseRay(DragDropRect.midpoint(), 5000.0f);
	float angleOfView = nGfxServer2::Instance()->GetCamera().GetAngleOfView();

	nArray<Graphics::PEntity> GfxEntities;
	//DragDropSelect(Ray.end(),
	//	nGfxServer2::Instance()->GetCamera().GetAngleOfView() * DragDropRect.width(),
	//	DragDropRect.width() / DragDropRect.height(),
	//	GfxEntities);

		CCameraEntity* cameraEntity = CurrLevel->GetCamera();
		matrix44 transform = cameraEntity->GetTransform();
		transform.lookatRh(lookAt, vector3(0.0f, 1.0f, 0.0f));
		nCamera2 camera = cameraEntity->GetCamera();
		camera.SetAngleOfView(angleOfView);
		camera.SetAspectRatio(aspectRatio);

		Ptr<CCameraEntity> dragDropCameraEntity = CCameraEntity::Create();
		dragDropCameraEntity->SetTransform(transform);
		dragDropCameraEntity->SetCamera(camera);

		//!!!uncomment & rewrite after new quadtree start working!

		//CurrLevel->GetRootCell()->ClearLinks(CEntity::PickupLink);
		//CurrLevel->GetRootCell()->UpdateLinks(dragDropCameraEntity, CEntity::GFXShape, CEntity::PickupLink);
		//for (int i = 0; i < dragDropCameraEntity->GetNumLinks(CEntity::PickupLink); i++)
		//	entities.PushBack(dragDropCameraEntity->GetLinkAt(CEntity::PickupLink, i));

	for (int i = 0; i < GfxEntities.GetCount(); i++)
	{
		CStrID UID = GfxEntities[i]->GetUserData();
		if (UID.IsValid())
		{
			n_assert(EntityMgr->ExistsEntityByID(UID));
			Entities.Append(EntityMgr->GetEntity(UID));
		}
	}
}
//---------------------------------------------------------------------
*/

nArray<PEntity> CEnvQueryManager::GetEntitiesInSphere(const vector3& MidPoint, float Radius)
{
	nArray<PEntity> GameEntities;
	Physics::CFilterSet ExcludeSet;
	nArray<Ptr<Physics::CEntity>> PhysEntities;
    PhysicsSrv->GetEntitiesInSphere(MidPoint, Radius, ExcludeSet, PhysEntities);

	for (int i = 0; i < PhysEntities.GetCount(); i++)
		if (EntityMgr->ExistsEntityByID(PhysEntities[i]->GetUserData()))
			GameEntities.Append(EntityMgr->GetEntity(PhysEntities[i]->GetUserData()));

	return GameEntities;
}
//---------------------------------------------------------------------

nArray<PEntity> CEnvQueryManager::GetEntitiesInBox(const vector3& Scale, const matrix44& m)
{
	nArray<PEntity> GameEntities;
	Physics::CFilterSet ExcludeSet;
	nArray<Ptr<Physics::CEntity>> PhysEntities;
	PhysicsSrv->GetEntitiesInBox(Scale, m, ExcludeSet, PhysEntities);

	for (int i = 0; i < PhysEntities.GetCount(); i++)
		if (EntityMgr->ExistsEntityByID(PhysEntities[i]->GetUserData()))
			GameEntities.Append(EntityMgr->GetEntity(PhysEntities[i]->GetUserData()));

	return GameEntities;
}
//---------------------------------------------------------------------

vector2 CEnvQueryManager::GetEntityScreenPositionRel(const Game::CEntity& Entity, const vector3* Offset)
{
	Scene::CCamera& Camera = *SceneSrv->GetCurrentScene()->GetMainCamera();

	vector3 EntityPos = Entity.Get<matrix44>(Attr::Transform).Translation();
	if (Offset) EntityPos += *Offset;
	vector4 WorldPos = Camera.GetViewMatrix() * EntityPos;
	WorldPos.w = 0.f;
	WorldPos = Camera.GetProjMatrix() * WorldPos;
	WorldPos = WorldPos * (1.f / WorldPos.w);
	return vector2(WorldPos.x * 0.5f + 0.5f, 0.5f - WorldPos.y * 0.5f);
}
//---------------------------------------------------------------------

vector2 CEnvQueryManager::GetEntityScreenPositionUpper(const Game::CEntity& Entity)
{
	Properties::CPropSceneNode* pNode = Entity.GetProperty<Properties::CPropSceneNode>();
	if (!pNode) return vector2::zero;

	bbox3 AABB;
	pNode->GetAABB(AABB);

	vector3 Offset(0.0f, AABB.size().y, 0.0f);
	return GetEntityScreenPositionRel(Entity, &Offset);
}
//---------------------------------------------------------------------

rectangle CEnvQueryManager::GetEntityScreenRectangle(const Game::CEntity& Entity, const vector3* const Offset)
{
	bbox3 AABB;

	Properties::CPropSceneNode* pNode = Entity.GetProperty<Properties::CPropSceneNode>();
	if (pNode) pNode->GetAABB(AABB);
	else
	{
		Properties::CPropAbstractPhysics* pPhysics = Entity.GetProperty<Properties::CPropAbstractPhysics>();
		pPhysics->GetAABB(AABB);
	}

	if (Offset)
	{
		AABB.vmax += *Offset;
		AABB.vmin += *Offset;
	}

	Scene::CCamera& Camera = *SceneSrv->GetCurrentScene()->GetMainCamera();
	AABB.transform(Camera.GetViewMatrix());

	rectangle Result;

	for (int i = 0; i < 6; i++)
	{
		vector3 Corner = AABB.corner_point(i);
		vector4 Pos(Corner.x, Corner.y, Corner.z, 0.0f);
		Pos = Camera.GetProjMatrix() * Pos;
		Pos /= Pos.w;
		
		vector2 ScreenPos = vector2(Pos.x * 0.5f + 0.5f, 0.5f - Pos.y * 0.5f);
		if (i > 0)
		{
			if (ScreenPos.x < Result.v0.x)
				Result.v0.x = ScreenPos.x;
			else if (ScreenPos.x > Result.v1.x)
				Result.v1.x = ScreenPos.x;

			if (ScreenPos.y < Result.v0.y)
				Result.v0.y = ScreenPos.y;
			else if (ScreenPos.y > Result.v1.y)
				Result.v1.y = ScreenPos.y;
		}
		else Result.v0 = Result.v1 = ScreenPos;
	}

	return Result;

	/*AABB.transform(nGfxServer2::Instance()->GetCamera().GetProjection());
	return rectangle(vector2(AABB.vmin.x * 0.5f + 0.5f, 0.5f - AABB.vmax.y * 0.5f), vector2(AABB.vmax.x * 0.5f + 0.5f, 0.5f - AABB.vmin.y * 0.5f));*/
	//return vector2(WorldPos.x * 0.5f + 0.5f, 0.5f - WorldPos.y * 0.5f);
}
//---------------------------------------------------------------------

bool CEnvQueryManager::GetEnvInfoAt(const vector3& Position, CEnvInfo& Info, float ProbeLength, int SelfPhysicsID) const
{
	n_assert(ProbeLength > 0);
	vector3 Dir(0.0f, -ProbeLength, 0.0f);

	Physics::CFilterSet ExcludeSet;
	if (SelfPhysicsID != -1) ExcludeSet.AddEntityID(SelfPhysicsID);
	const Physics::CContactPoint* pContact = PhysicsSrv->GetClosestContactAlongRay(Position, Dir, &ExcludeSet);
	if (pContact)
	{
		Info.WorldHeight = pContact->Position.y;
		Info.Material = pContact->Material;
		OK;
	}
	else FAIL;
}
//---------------------------------------------------------------------

bool CEnvQueryManager::OnFrame(const Events::CEventBase& Event)
{
	CStrID OldEntityUnderMouse = EntityUnderMouse;

    // Get 3d contact under mouse
    if (!UISrv->IsMouseOverGUI())
	{
		float XRel, YRel;
		InputSrv->GetMousePosRel(XRel, YRel);
		line3 Ray;
		SceneSrv->GetCurrentScene()->GetMainCamera()->GetRay3D(XRel, YRel, 5000.f, Ray);
		const Physics::CContactPoint* pContact = PhysicsSrv->GetClosestContactAlongRay(Ray.start(), Ray.vec());
        MouseIntersection = (pContact != NULL);
        if (MouseIntersection)
        {
            // Store intersection position
            MousePos3D = pContact->Position;
            UpVector = pContact->UpVector;

            // Get entity under mouse
            Physics::CEntity* pPhysEntity = PhysicsSrv->FindEntityByUniqueID(pContact->EntityID);
			EntityUnderMouse = (pPhysEntity) ? pPhysEntity->GetUserData() : CStrID::Empty;
        }
		else
		{
			// Reset values
			EntityUnderMouse = CStrID::Empty;
			MousePos3D.set(0.0f, 0.0f, 0.0f);
		}
    }
	else
	{
		MouseIntersection = false;
		EntityUnderMouse = CStrID::Empty;
		MousePos3D.set(0.0f, 0.0f, 0.0f);
	}

	if (OldEntityUnderMouse != EntityUnderMouse)
	{
		Game::CEntity* pEntityUnderMouse = EntityMgr->GetEntity(OldEntityUnderMouse);
		if (pEntityUnderMouse)
		{
			PParams P = n_new(CParams);
			P->Set(CStrID("IsOver"), false);
			pEntityUnderMouse->FireEvent(CStrID("ObjMouseOver"), P);
		}
		
		pEntityUnderMouse = GetEntityUnderMouse();
		if (pEntityUnderMouse)
		{
			PParams P = n_new(CParams);
			P->Set(CStrID("IsOver"), true);
			pEntityUnderMouse->FireEvent(CStrID("ObjMouseOver"), P);
		}
	}

	OK;
}
//---------------------------------------------------------------------

} // namespace Game
