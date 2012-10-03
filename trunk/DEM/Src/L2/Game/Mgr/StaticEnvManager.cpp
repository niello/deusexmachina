#include "StaticEnvManager.h"

#include <Physics/Level.h>
#include <Physics/Event/SetTransform.h>
#include <Gfx/GfxServer.h>
#include <Gfx/ShapeEntity.h>
#include <Data/DataServer.h>
#include <Data/DataArray.h>
#include <Game/Mgr/EntityManager.h>
#include <DB/ValueTable.h>

namespace Attr
{
	DeclareAttr(GUID);
	DeclareAttr(Physics);
	DeclareAttr(Graphics);
	DeclareAttr(Transform);
	DeclareAttr(AnimPath);
}

namespace Game
{
__ImplementClass(CStaticEnvManager, 'MENV', Game::CManager);
__ImplementSingleton(CStaticEnvManager);

using namespace Physics;

CStaticEnvManager::CStaticEnvManager()
{
	__ConstructSingleton;
}
//---------------------------------------------------------------------

CStaticEnvManager::~CStaticEnvManager()
{
	__DestructSingleton;
}
//---------------------------------------------------------------------

bool CStaticEnvManager::AddEnvObject(const DB::PValueTable& Table, int RowIdx)
{
	n_assert(Table.isvalid());
	n_assert(RowIdx != INVALID_INDEX);

	// If the AnimPath attribute exists, create an animated entity
	if (Table->Get<nString>(Attr::AnimPath, RowIdx).IsValid()) FAIL;

	CEnvObject* pObj = NULL;
	const matrix44& EntityTfm = Table->Get<matrix44>(Attr::Transform, RowIdx);

	const nString& CompositeName = Table->Get<nString>(Attr::Physics, RowIdx);    
	if (CompositeName.IsValid())
	{
		PParams Desc = DataSrv->LoadPRM(nString("physics:") + CompositeName + ".prm");
		int Idx = Desc->IndexOf(CStrID("Bodies"));

		// If is not a pure collide object, load as a physics entity
		if (Idx != INVALID_INDEX && Desc->Get<PDataArray>(Idx)->Size() > 0) FAIL;

		Idx = Desc->IndexOf(CStrID("Shapes"));
		if (Idx != INVALID_INDEX)
		{
			CDataArray& Shapes = *Desc->Get<PDataArray>(Idx);
			if (Shapes.Size() > 0)
			{
				CStrID UID = Table->Get<CStrID>(Attr::GUID, RowIdx);
				EnvObjects.Add(UID, CEnvObject()); //!!!unnecessary copying!
				pObj = &EnvObjects[UID];
				//InvEntityTfm.invert();
				
				for (int i = 0; i < Shapes.Size(); i++)
				{
					PParams ShapeDesc = Shapes[i];
					PShape pShape = (CShape*)CoreFct->Create("Physics::C" + ShapeDesc->Get<nString>(CStrID("Type")));
					pShape->Init(ShapeDesc);
					pObj->CollLocalTfm.Append(pShape->GetTransform());// * InvEntityTfm);
					pShape->SetTransform(pShape->GetTransform() * EntityTfm);
					pObj->Collision.Append(pShape);
					PhysicsSrv->GetLevel()->AttachShape(pShape);
				}
			}
		}
	}

	const nString& GfxResName = Table->Get<nString>(Attr::Graphics, RowIdx);    
	if (GfxResName.IsValid())
	{
		nArray<Graphics::PShapeEntity> GfxEntities;
		GfxSrv->CreateGfxEntities(GfxResName, EntityTfm, GfxEntities);
		
		if (GfxEntities.Size() > 0)
		{
			if (!pObj)
			{
				CStrID UID = Table->Get<CStrID>(Attr::GUID, RowIdx);
				EnvObjects.Add(UID, CEnvObject()); //!!!unnecessary copying!
				pObj = &EnvObjects[UID];
			}

			matrix44 InvEntityTfm = EntityTfm;
			InvEntityTfm.invert();

			for (int i = 0; i < GfxEntities.Size(); i++)
			{
				Graphics::PShapeEntity pEnt = GfxEntities[i];
				pObj->Gfx.Append(pEnt);
				pObj->GfxLocalTfm.Append(pEnt->GetTransform() * InvEntityTfm);
				GfxSrv->GetLevel()->AttachEntity(pEnt);
			}
		}
	}

	OK;
}
//---------------------------------------------------------------------

bool CStaticEnvManager::EnvObjectExists(CStrID ID) const
{
	n_assert(ID.IsValid());
	return EnvObjects.Contains(ID) || EntityMgr->ExistsEntityByID(ID);
}
//---------------------------------------------------------------------

bool CStaticEnvManager::IsEntityStatic(CStrID ID) const
{
	n_assert(ID.IsValid());
	if (EnvObjects.Contains(ID)) OK;
	CEntity* pEnt = EntityMgr->GetEntityByID(ID);
	return pEnt ? IsEntityStatic(*pEnt) : false;
}
//---------------------------------------------------------------------

bool CStaticEnvManager::IsEntityStatic(CEntity& Entity) const
{
	//!!!unnecessary data copying!
	nString Value;
	if (Entity.Get<nString>(Attr::AnimPath, Value) && Value.IsValid()) FAIL;

	if (Entity.Get<nString>(Attr::Physics, Value) && Value.IsValid())
	{
		// It uses HRD cache, so it isn't so slow
		PParams Desc = DataSrv->LoadPRM(nString("physics:") + Value + ".prm");
		int Idx = Desc->IndexOf(CStrID("Bodies"));
		if (Idx != INVALID_INDEX && Desc->Get<PDataArray>(Idx)->Size() > 0) FAIL;
	}

	OK;
}
//---------------------------------------------------------------------

void CStaticEnvManager::SetEnvObjectTransform(CStrID ID, const matrix44& Tfm)
{
	n_assert(ID.IsValid());

	int Idx = EnvObjects.FindIndex(ID);
	if (Idx != INVALID_INDEX)
	{
		CEnvObject& Obj = EnvObjects.ValueAtIndex(Idx);
		for (int i = 0; i < Obj.Collision.Size(); i++)
			Obj.Collision[i]->SetTransform(Obj.CollLocalTfm[i] * Tfm);
		for (int i = 0; i < Obj.Collision.Size(); i++)
			Obj.Gfx[i]->SetTransform(Obj.GfxLocalTfm[i] * Tfm);
	}
	else
	{
		// Normal entity case
		Game::PEntity pEnt = EntityMgr->GetEntityByID(ID);
		if (pEnt.isvalid())
			pEnt->FireEvent(Event::SetTransform(Tfm));
	}
}
//---------------------------------------------------------------------

void CStaticEnvManager::DeleteEnvObject(CStrID ID)
{
	n_assert(ID.IsValid());

	int Idx = EnvObjects.FindIndex(ID);
	if (Idx != INVALID_INDEX)
	{
		CEnvObject& Obj = EnvObjects.ValueAtIndex(Idx);
		for (int j = 0; j < Obj.Collision.Size(); j++)
			PhysicsSrv->GetLevel()->RemoveShape(Obj.Collision[j]);
		for (int j = 0; j < Obj.Collision.Size(); j++)
			GfxSrv->GetLevel()->RemoveEntity(Obj.Gfx[j]);
		EnvObjects.EraseAt(Idx);
	}
	else
	{
		Game::PEntity pEnt = EntityMgr->GetEntityByID(ID);
		if (pEnt.isvalid()) EntityMgr->RemoveEntity(pEnt);
	}
}
//---------------------------------------------------------------------

void CStaticEnvManager::ClearStaticEnv()
{
	for (int i = 0; i < EnvObjects.Size(); i++)
	{
		CEnvObject& Obj = EnvObjects.ValueAtIndex(i);
		for (int j = 0; j < Obj.Collision.Size(); j++)
			PhysicsSrv->GetLevel()->RemoveShape(Obj.Collision[j]);
		for (int j = 0; j < Obj.Collision.Size(); j++)
			GfxSrv->GetLevel()->RemoveEntity(Obj.Gfx[j]);
	}
	EnvObjects.Clear();
}
//---------------------------------------------------------------------

} // namespace Game
