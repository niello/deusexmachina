#include "StaticObject.h"

#include <Game/GameLevel.h>
#include <Game/SceneNodeValidateAttrs.h>
#include <Resources/Resource.h>
#include <Resources/ResourceManager.h>
#include <Scene/SceneNode.h>
#include <Scene/SceneNodeLoaderSCN.h>
#include <Physics/CollisionObjStatic.h>
#include <Physics/PhysicsLevel.h>
#include <Data/DataServer.h>
#include <Data/DataArray.h>
#include <IO/PathUtils.h>

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
	bool CreateNode = Desc->Get<CString>(NodeFile, CStrID("SceneFile"));

	const char* pUnresolved;
	Scene::PSceneNode PathNode = Level->GetSceneRoot()->FindDeepestChild(NodePath.CStr(), pUnresolved);
	ExistingNode = !pUnresolved;
	if (pUnresolved) PathNode = PathNode->CreateChildChain(pUnresolved);
	n_assert(PathNode.IsValidPtr());

	if (CreateNode)
	{
		// Create node at runtime

		if (NodeFile.IsValid())
		{
			CString RsrcURI = "Scene:" + NodeFile + ".scn";
			Resources::PResource Rsrc = ResourceMgr->RegisterResource(RsrcURI.CStr());
			if (!Rsrc->IsLoaded())
			{
				Resources::PResourceLoader Loader = Rsrc->GetLoader();
				if (Loader.IsNullPtr())
					Loader = ResourceMgr->CreateDefaultLoaderFor<Scene::CSceneNode>(PathUtils::GetExtension(RsrcURI.CStr()));
				ResourceMgr->LoadResourceSync(*Rsrc, *Loader);
				n_assert(Rsrc->IsLoaded());
			}
			Node = Rsrc->GetObject<Scene::CSceneNode>()->Clone(true);
			PathNode->AddChild(UID, *Node.GetUnsafe());
		}
		else
		{
			Node = PathNode->CreateChild(UID);
		}

		Game::CSceneNodeValidateAttrs Visitor;
		Visitor.Level = Level;
		Visitor.Visit(*Node.GetUnsafe());
	}
	else
	{
		// Use node created in a tool. It was already validated.
		Node = PathNode;
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