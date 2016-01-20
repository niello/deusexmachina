#include "StaticObject.h"

#include <Game/GameLevel.h>
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
	n_assert(Desc.IsNullPtr());

	Data::PParams Attrs;
	n_verify_dbg(ObjDesc.Get(Attrs, CStrID("Attrs")));

	Desc = Attrs; //&ObjDesc;

	CString NodePath;
	Desc->Get<CString>(NodePath, CStrID("ScenePath"));
	CString NodeFile;
	Desc->Get<CString>(NodeFile, CStrID("SceneFile"));

	if (NodePath.IsEmpty() && NodeFile.IsValid())
		NodePath = UID.CStr();

	if (NodePath.IsValid())
	{
		//???optimize duplicate search?
		Node = Level->GetSceneNode(NodePath.CStr(), false);
		ExistingNode = Node.IsValidPtr();
		if (!ExistingNode) Node = Level->GetSceneNode(NodePath.CStr(), true);
		n_assert(Node.IsValidPtr());

		if (NodeFile.IsValid()) n_verify(Scene::LoadNodesFromSCN("Scene:" + NodeFile + ".scn", Node));
	}

	const matrix44& EntityTfm = Desc->Get<matrix44>(CStrID("Transform"));

	if (!ExistingNode) SetTransform(EntityTfm);

	// Update child nodes' world transform recursively. There are no controllers, so update is finished.
	// It is necessary because collision objects may require subnode world transformations.
	if (Node.IsValidPtr()) Node->UpdateTransform(NULL, 0, true, NULL);

	const CString& PhysicsDescFile = Desc->Get<CString>(CStrID("Physics"), CString::Empty);    
	if (PhysicsDescFile.IsValid() && Level->GetPhysics())
	{
		Data::PParams PhysicsDesc = DataSrv->LoadPRM(CString("Physics:") + PhysicsDescFile.CStr() + ".prm");
		if (PhysicsDesc.IsValidPtr())
		{
			const Data::CDataArray& Objects = *PhysicsDesc->Get<Data::PDataArray>(CStrID("Objects"));
			for (UPTR i = 0; i < Objects.GetCount(); ++i)
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
	if (CollObj.IsValidPtr())
	{
		CollObj->RemoveFromLevel();
		CollObj = NULL;
	}

	if (Node.IsValidPtr() && !ExistingNode)
	{
		Node->Remove();
		Node = NULL;
	}

	Desc = NULL;
}
//---------------------------------------------------------------------

void CStaticObject::SetTransform(const matrix44& Tfm)
{
	if (CollObj.IsValidPtr()) CollObj->SetTransform(Tfm);
	if (Node.IsValidPtr()) Node->SetWorldTransform(Tfm);
}
//---------------------------------------------------------------------

}