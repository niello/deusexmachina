#include "StaticObject.h"

#include <Game/GameLevel.h>
#include <Scene/Scene.h>
#include <Physics/CollisionObjStatic.h>
#include <Physics/PhysicsWorld.h>
#include <Data/DataServer.h>
#include <Data/DataArray.h>

namespace Scene
{
	bool LoadNodesFromSCN(const CString& FileName, PSceneNode RootNode);
}

namespace Game
{
__ImplementClassNoFactory(Game::CStaticObject, Core::CObject);

CStaticObject::CStaticObject(CStrID _UID, CGameLevel& _Level): UID(_UID), Level(&_Level)
{
}
//---------------------------------------------------------------------

CStaticObject::~CStaticObject()
{
}
//---------------------------------------------------------------------

void CStaticObject::SetUID(CStrID NewUID)
{
	n_assert(NewUID.IsValid());
	if (UID == NewUID) return;
	UID = NewUID;
}
//---------------------------------------------------------------------

void CStaticObject::Init(const Data::CParams& ObjDesc)
{
	n_assert(!Desc.IsValid());

	Data::PParams Attrs;
	n_verify_dbg(ObjDesc.Get(Attrs, CStrID("Attrs")));

	Desc = Attrs; //&ObjDesc;

	CString NodePath;
	Desc->Get<CString>(NodePath, CStrID("ScenePath"));
	CString NodeFile;
	Desc->Get<CString>(NodeFile, CStrID("SceneFile"));

	if (NodePath.IsEmpty() && NodeFile.IsValid())
		NodePath = UID.CStr();

	if (NodePath.IsValid() && Level->GetScene())
	{
		//???optimize duplicate search?
		Node = Level->GetScene()->GetNode(NodePath.CStr(), false);
		ExistingNode = Node.IsValid();
		if (!ExistingNode) Node = Level->GetScene()->GetNode(NodePath.CStr(), true);
		n_assert(Node.IsValid());

		if (NodeFile.IsValid()) n_verify(Scene::LoadNodesFromSCN("Scene:" + NodeFile + ".scn", Node));
	}

	const matrix44& EntityTfm = Desc->Get<matrix44>(CStrID("Transform"));

	if (!ExistingNode) SetTransform(EntityTfm);

	// Update child nodes' world transform recursively. There are no controllers, so update is finished.
	// It is necessary because collision objects may require subnode world transformations.
	if (Node.IsValid()) Node->UpdateLocalSpace();

	const CString& PhysicsDescFile = Desc->Get<CString>(CStrID("Physics"), NULL);    
	if (PhysicsDescFile.IsValid() && Level->GetPhysics())
	{
		Data::PParams PhysicsDesc = DataSrv->LoadPRM(CString("Physics:") + PhysicsDescFile.CStr() + ".prm");
		if (PhysicsDesc.IsValid())
		{
			const Data::CDataArray& Objects = *PhysicsDesc->Get<Data::PDataArray>(CStrID("Objects"));
			for (int i = 0; i < Objects.GetCount(); ++i)
			{
				//???allow moving collision objects and rigid bodies?

				const Data::CParams& ObjDesc = *Objects.Get<Data::PParams>(i);
				CollObj = n_new(Physics::CCollisionObjStatic);
				CollObj->Init(ObjDesc); //???where to get offset?

				Scene::CSceneNode* pCurrNode = Node.GetUnsafe();
				const CString& RelNodePath = ObjDesc.Get<CString>(CStrID("Node"), CString::Empty);
				if (pCurrNode && RelNodePath.IsValid())
				{
					pCurrNode = pCurrNode->GetChild(RelNodePath.CStr());
					n_assert2_dbg(pCurrNode && "Child node not found", RelNodePath.CStr());
				}

				CollObj->SetTransform(pCurrNode ? pCurrNode->GetWorldMatrix() : EntityTfm);
				CollObj->AttachToLevel(*Level->GetPhysics());
			}
		}
	}
}
//---------------------------------------------------------------------

void CStaticObject::Term()
{
	if (CollObj.IsValid())
	{
		CollObj->RemoveFromLevel();
		CollObj = NULL;
	}

	if (Node.IsValid() && !ExistingNode)
	{
		Node->RemoveFromParent();
		Node = NULL;
	}

	Desc = NULL;
}
//---------------------------------------------------------------------

void CStaticObject::SetTransform(const matrix44& Tfm)
{
	if (CollObj.IsValid()) CollObj->SetTransform(Tfm);
	if (Node.IsValid()) Node->SetWorldTransform(Tfm);
}
//---------------------------------------------------------------------

} // namespace Game
