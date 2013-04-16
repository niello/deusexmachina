#include "EnvQueryManager.h"

#include <UI/UIServer.h>
#include <Data/Params.h>
#include <Physics/PhysicsServer.h>
#include <Input/InputServer.h>
#include <Game/Mgr/EntityManager.h>
#include <Events/EventManager.h>
#include <Physics/Prop/PropAbstractPhysics.h>

namespace Game
{
ImplementRTTI(Game::CEnvQueryManager, Game::CManager);
ImplementFactory(Game::CEnvQueryManager);
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
	return EntityMgr->GetEntityByID(EntityUnderMouse);
}
//---------------------------------------------------------------------

/*
void CEnvQueryManager::GetEntitiesUnderMouseDragDropRect(const rectangle& DragDropRect,
														 nArray<PEntity>& Entities)
{
	line3 Ray = nGfxServer2::Instance()->ComputeWorldMouseRay(DragDropRect.midpoint(), 5000.0f);
	float angleOfView = nGfxServer2::Instance()->GetCamera().GetAngleOfView();

	nArray<Graphics::PEntity> GfxEntities;
	//RenderSrv->GetDisplay().DragDropSelect(Ray.end(),
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

	for (int i = 0; i < GfxEntities.Size(); i++)
	{
		CStrID UID = GfxEntities[i]->GetUserData();
		if (UID.IsValid())
		{
			n_assert(EntityMgr->ExistsEntityByID(UID));
			Entities.Append(EntityMgr->GetEntityByID(UID));
		}
	}
}
//---------------------------------------------------------------------

nArray<PEntity> CEnvQueryManager::GetEntitiesInSphere(const vector3& MidPoint, float Radius)
{
	nArray<PEntity> GameEntities;
	Physics::CFilterSet ExcludeSet;
	nArray<Ptr<Physics::CEntity>> PhysEntities;
    PhysicsSrv->GetEntitiesInSphere(MidPoint, Radius, ExcludeSet, PhysEntities);

	for (int i = 0; i < PhysEntities.Size(); i++)
		if (EntityMgr->ExistsEntityByID(PhysEntities[i]->GetUserData()))
			GameEntities.Append(EntityMgr->GetEntityByID(PhysEntities[i]->GetUserData()));

	return GameEntities;
}
//---------------------------------------------------------------------

nArray<PEntity> CEnvQueryManager::GetEntitiesInBox(const vector3& Scale, const matrix44& m)
{
	nArray<PEntity> GameEntities;
	Physics::CFilterSet ExcludeSet;
	nArray<Ptr<Physics::CEntity>> PhysEntities;
	PhysicsSrv->GetEntitiesInBox(Scale, m, ExcludeSet, PhysEntities);

	for (int i = 0; i < PhysEntities.Size(); i++)
		if (EntityMgr->ExistsEntityByID(PhysEntities[i]->GetUserData()))
			GameEntities.Append(EntityMgr->GetEntityByID(PhysEntities[i]->GetUserData()));

	return GameEntities;
}
//---------------------------------------------------------------------
*/

vector2 CEnvQueryManager::GetEntityScreenPositionRel(const Game::CEntity& pEntity, const vector3* Offset)
{
	//GFX
	vector3 EntityPos = pEntity.Get<matrix44>(Attr::Transform).pos_component();
	if (Offset) EntityPos += *Offset;
	vector4 WorldPos; //= nGfxServer2::Instance()->GetTransform(nGfxServer2::View) * EntityPos;
	WorldPos.w = 0;
	//WorldPos = nGfxServer2::Instance()->GetCamera().GetProjection() * WorldPos;
	WorldPos = WorldPos * (1.f / WorldPos.w);
	return vector2(WorldPos.x * 0.5f + 0.5f, 0.5f - WorldPos.y * 0.5f);
}
//---------------------------------------------------------------------

vector2 CEnvQueryManager::GetEntityScreenPositionUpper(const Game::CEntity& pEntity)
{
	//GFX
	//Properties::CPropGraphics* pGraphics = pEntity.FindProperty<Properties::CPropGraphics>();
	//n_assert(pGraphics);

	//bbox3 AABB;
	//pGraphics->GetAABB(AABB);

	//vector3 Offset(0.0f, AABB.size().y, 0.0f);
	return GetEntityScreenPositionRel(pEntity, NULL); //&Offset);
}
//---------------------------------------------------------------------

rectangle CEnvQueryManager::GetEntityScreenRectangle(const Game::CEntity& Entity, const vector3* const Offset)
{
	//GFX
	/*
	bbox3 AABB;

	Properties::CPropGraphics* pGraphics = Entity.FindProperty<Properties::CPropGraphics>();
	if (pGraphics) pGraphics->GetAABB(AABB);
	else
	{
		Properties::CPropAbstractPhysics* pPhysics = Entity.FindProperty<Properties::CPropAbstractPhysics>();
		pPhysics->GetAABB(AABB);
	}

	if (Offset)
	{
		AABB.vmax += *Offset;
		AABB.vmin += *Offset;
	}

	AABB.transform(nGfxServer2::Instance()->GetTransform(nGfxServer2::View));

	const matrix44& Projection = nGfxServer2::Instance()->GetCamera().GetProjection();
	*/
	rectangle Result;

	/*
	for (int i = 0; i < 6; i++)
	{
		vector3 Corner = AABB.corner_point(i);
		vector4 Pos(Corner.x, Corner.y, Corner.z, 0.0f);
		Pos = Projection * Pos;
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
*/
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
        const Physics::CContactPoint* pContact =
			PhysicsSrv->GetClosestContactUnderMouse(InputSrv->GetMousePosRel(), 5000.0f);
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
		Game::CEntity* pEntityUnderMouse = EntityMgr->GetEntityByID(OldEntityUnderMouse);
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
