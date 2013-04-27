#include "StaticEnvManager.h"

#include <Physics/Level.h>
#include <Physics/Event/SetTransform.h>
#include <Scene/SceneServer.h>
#include <Data/DataServer.h>
#include <Data/DataArray.h>
#include <Game/Mgr/EntityManager.h>
#include <DB/ValueTable.h>

namespace Attr
{
	DeclareAttr(GUID);
	DeclareAttr(Physics);
	DeclareAttr(AnimDesc);
	DeclareAttr(Transform);
	DeclareAttr(ScenePath);
	DeclareAttr(SceneFile);
}

namespace Scene
{
	bool LoadNodesFromSCN(const nString& FileName, PSceneNode RootNode, bool PreloadResources = true);
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

	nString NodePath = Table->Get<nString>(Attr::ScenePath, RowIdx);
	const nString& NodeRsrc = Table->Get<nString>(Attr::SceneFile, RowIdx);

	if (NodePath.IsEmpty() && NodeRsrc.IsValid())
		NodePath = Table->Get<CStrID>(Attr::GUID, RowIdx).CStr();
	
	if (NodePath.IsValid())
	{
		if (!pObj)
		{
			CStrID UID = Table->Get<CStrID>(Attr::GUID, RowIdx);
			EnvObjects.Add(UID, CEnvObject()); //!!!unnecessary copying!
			pObj = &EnvObjects[UID];
		}

		//???optimize duplicate search?
		pObj->Node = SceneSrv->GetCurrentScene()->GetNode(NodePath.Get(), false);
		pObj->ExistingNode = pObj->Node.isvalid();
		if (!pObj->ExistingNode) pObj->Node = SceneSrv->GetCurrentScene()->GetNode(NodePath.Get(), true);
		n_assert(pObj->Node.isvalid());

		if (NodeRsrc.IsValid()) n_assert(Scene::LoadNodesFromSCN("scene:" + NodeRsrc + ".scn", pObj->Node));

		if (!pObj->ExistingNode) pObj->Node->SetLocalTransform(Table->Get<matrix44>(Attr::Transform, RowIdx)); //???set local? or set global & then calc local?
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

	// We have physics bodies that can move us
	if (Entity.Get<nString>(Attr::Physics, Value) && Value.IsValid())
	{
		// It uses HRD cache, so it isn't so slow
		PParams Desc = DataSrv->LoadPRM(nString("physics:") + Value + ".prm");
		int Idx = Desc->IndexOf(CStrID("Bodies"));
		if (Idx != INVALID_INDEX && Desc->Get<PDataArray>(Idx)->Size() > 0) FAIL;
	}

	// We have animations that can move us
	if (Entity.Get<nString>(Attr::AnimDesc, Value) && Value.IsValid())
		FAIL;

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
		//if (Obj.Node.isvalid()) Obj.Node->SetLocalTransform(Tfm); //???!!!setglobal!
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
		if (Obj.Node.isvalid() && !Obj.ExistingNode) Obj.Node->RemoveFromParent();
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
		if (Obj.Node.isvalid() && !Obj.ExistingNode) Obj.Node->RemoveFromParent();
	}
	EnvObjects.Clear();
}
//---------------------------------------------------------------------

} // namespace Game
