#include "StaticEnvManager.h"

#include <Physics/PhysicsLevel.h>
#include <Physics/Event/SetTransform.h>
#include <Scene/SceneServer.h>
#include <Data/DataServer.h>
#include <Data/DataArray.h>
#include <Game/EntityManager.h>
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
__ImplementClassNoFactory(Game::CStaticEnvManager, Core::CRefCounted);
__ImplementSingleton(CStaticEnvManager);

using namespace Physics;

bool CStaticEnvManager::AddEnvObject(const DB::PValueTable& Table, int RowIdx)
{
	n_assert(Table.IsValid());
	n_assert(RowIdx != INVALID_INDEX);

	CEnvObject* pObj = NULL;
	//const matrix44& EntityTfm = Table->GetAttr<matrix44>(CStrID("Transform"), RowIdx);
	matrix44 EntityTfm;

	const nString& CompositeName = Table->Get<nString>(CStrID("Physics"), RowIdx);    
	if (CompositeName.IsValid())
	{
		PParams Desc = DataSrv->LoadPRM(nString("physics:") + CompositeName + ".prm");
		int Idx = Desc->IndexOf(CStrID("Bodies"));

		// If is not a pure collide object, load as a physics entity
		if (Idx != INVALID_INDEX && Desc->Get<PDataArray>(Idx)->GetCount() > 0) FAIL;

		Idx = Desc->IndexOf(CStrID("Shapes"));
		if (Idx != INVALID_INDEX)
		{
			CDataArray& Shapes = *Desc->Get<PDataArray>(Idx);
			if (Shapes.GetCount() > 0)
			{
				CStrID UID = Table->Get<CStrID>(Attr::GUID, RowIdx);
				EnvObjects.Add(UID, CEnvObject()); //!!!unnecessary copying!
				pObj = &EnvObjects[UID];
				//InvEntityTfm.invert();
				
				for (int i = 0; i < Shapes.GetCount(); i++)
				{
					PParams ShapeDesc = Shapes[i];
					PShape pShape = (CShape*)Factory->Create("Physics::C" + ShapeDesc->Get<nString>(CStrID("Type")));
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

		//!!!send level to loader!

		//???optimize duplicate search?
		pObj->Node = SceneSrv->GetCurrentScene()->GetNode(NodePath.CStr(), false);
		pObj->ExistingNode = pObj->Node.IsValid();
		if (!pObj->ExistingNode) pObj->Node = SceneSrv->GetCurrentScene()->GetNode(NodePath.CStr(), true);
		n_assert(pObj->Node.IsValid());

		if (NodeRsrc.IsValid()) n_assert(Scene::LoadNodesFromSCN("scene:" + NodeRsrc + ".scn", pObj->Node));

		if (!pObj->ExistingNode) pObj->Node->SetLocalTransform(Table->GetAttr<matrix44>(CStrID("Transform"), RowIdx)); //???set local? or set global & then calc local?
	}

	OK;
}
//---------------------------------------------------------------------

bool CStaticEnvManager::EnvObjectExists(CStrID ID) const
{
	n_assert(ID.IsValid());
	return EnvObjects.Contains(ID) || EntityMgr->EntityExists(ID);
}
//---------------------------------------------------------------------

bool CStaticEnvManager::IsEntityStatic(CStrID ID) const
{
	n_assert(ID.IsValid());
	if (EnvObjects.Contains(ID)) OK;
	CEntity* pEnt = EntityMgr->GetEntity(ID);
	return pEnt ? IsEntityStatic(*pEnt) : false;
}
//---------------------------------------------------------------------

bool CStaticEnvManager::IsEntityStatic(CEntity& Entity) const
{
	//!!!unnecessary data copying!
	nString Value;

	// We have physics bodies that can move us
	if (Entity.GetAttr<nString>(CStrID("Physics"), Value) && Value.IsValid())
	{
		// It uses HRD cache, so it isn't so slow
		PParams Desc = DataSrv->LoadPRM(nString("physics:") + Value + ".prm");
		int Idx = Desc->IndexOf(CStrID("Bodies"));
		if (Idx != INVALID_INDEX && Desc->Get<PDataArray>(Idx)->GetCount() > 0) FAIL;
	}

	// We have animations that can move us
	if (Entity.GetAttr<nString>(CStrID("AnimDesc"), Value) && Value.IsValid())
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
		for (int i = 0; i < Obj.Collision.GetCount(); i++)
			Obj.Collision[i]->SetTransform(Obj.CollLocalTfm[i] * Tfm);
		//if (Obj.Node.IsValid()) Obj.Node->SetLocalTransform(Tfm); //???!!!setglobal!
	}
	else
	{
		// Normal entity case
		Game::PEntity pEnt = EntityMgr->GetEntity(ID);
		if (pEnt.IsValid())
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
		for (int j = 0; j < Obj.Collision.GetCount(); j++)
			PhysicsSrv->GetLevel()->RemoveShape(Obj.Collision[j]);
		if (Obj.Node.IsValid() && !Obj.ExistingNode) Obj.Node->RemoveFromParent();
		EnvObjects.EraseAt(Idx);
	}
	else EntityMgr->DeleteEntity(ID);
}
//---------------------------------------------------------------------

void CStaticEnvManager::ClearStaticEnv()
{
	for (int i = 0; i < EnvObjects.GetCount(); i++)
	{
		CEnvObject& Obj = EnvObjects.ValueAtIndex(i);
		for (int j = 0; j < Obj.Collision.GetCount(); j++)
			PhysicsSrv->GetLevel()->RemoveShape(Obj.Collision[j]);
		if (Obj.Node.IsValid() && !Obj.ExistingNode) Obj.Node->RemoveFromParent();
	}
	EnvObjects.Clear();
}
//---------------------------------------------------------------------

} // namespace Game
