#include "StaticObject.h"

#include <Game/GameLevel.h>
#include <Scene/Scene.h>
#include <Physics/Collision/Shape.h>
#include <Physics/CollisionObjStatic.h>
#include <Physics/PhysicsWorldOld.h>
#include <Physics/PhysicsWorld.h>
#include <Data/DataServer.h>
#include <Data/DataArray.h>

namespace Scene
{
	bool LoadNodesFromSCN(const nString& FileName, PSceneNode RootNode, bool PreloadResources = true);
}

namespace Game
{
__ImplementClassNoFactory(Game::CStaticObject, Core::CRefCounted);

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

void CStaticObject::Init(Data::CParams& ObjDesc)
{
	n_assert(!Desc.IsValid());

	Data::PParams Attrs;
	n_verify_dbg(ObjDesc.Get(Attrs, CStrID("Attrs")));

	Desc = Attrs; //&ObjDesc;

	nString NodePath;
	Desc->Get<nString>(NodePath, CStrID("ScenePath"));
	nString NodeFile;
	Desc->Get<nString>(NodeFile, CStrID("SceneFile"));

	if (NodePath.IsEmpty() && NodeFile.IsValid())
		NodePath = UID.CStr();

	if (NodePath.IsValid())
	{
		//???optimize duplicate search?
		Node = Level->GetScene()->GetNode(NodePath.CStr(), false);
		ExistingNode = Node.IsValid();
		if (!ExistingNode) Node = Level->GetScene()->GetNode(NodePath.CStr(), true);
		n_assert(Node.IsValid());

		if (NodeFile.IsValid()) n_assert(Scene::LoadNodesFromSCN("scene:" + NodeFile + ".scn", Node));
	}

	const nString& CompositeName = Desc->Get<nString>(CStrID("PhysicsOld"), NULL);    
	if (CompositeName.IsValid())
	{
		Data::PParams Desc = DataSrv->LoadPRM(nString("physics:") + CompositeName + ".prm");
		int Idx = Desc->IndexOf(CStrID("Shapes"));
		if (Idx != INVALID_INDEX)
		{
			Data::CDataArray& Shapes = *Desc->Get<Data::PDataArray>(Idx);
			for (int i = 0; i < Shapes.GetCount(); ++i)
			{
				Data::PParams ShapeDesc = Shapes[i];
				nString ShCls = ShapeDesc->Get<nString>(CStrID("Type"));
				if (ShCls == "HeightfieldShape") ShCls = "HeightfieldShapeOld";
				Physics::PShape pShape = (Physics::CShape*)Factory->Create("Physics::C" + ShCls);
				pShape->Init(ShapeDesc);
				CollLocalTfm.Append(pShape->GetTransform());
				Collision.Append(pShape);
				Level->GetPhysicsOld()->AttachShape(pShape);
				//???associate collision shape with game entity UID?
			}
		}
	}

	const matrix44& EntityTfm = Desc->Get<matrix44>(CStrID("Transform"));

	if (ExistingNode)
	{
		for (int i = 0; i < Collision.GetCount(); ++i)
			Collision[i]->SetTransform(CollLocalTfm[i] * Node->GetWorldMatrix());
	}
	else SetTransform(EntityTfm);

	// Update child nodes' world transform recursively. There are no controllers, so update is finished.
	// It is necessary because collision objects may require subnode world transformations.
	if (Node.IsValid()) Node->UpdateLocalSpace();

	const nString& PhysicsDescFile = Desc->Get<nString>(CStrID("Physics"), NULL);    
	if (PhysicsDescFile.IsValid() && Level->GetPhysics())
	{
		Data::PParams PhysicsDesc = DataSrv->LoadHRD(nString("physics:") + PhysicsDescFile.CStr() + ".hrd"); //!!!load prm!
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
				const nString& RelNodePath = ObjDesc.Get<nString>(CStrID("Node"), nString::Empty);
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
	for (int i = 0; i < Collision.GetCount(); ++i)
		Level->GetPhysicsOld()->RemoveShape(Collision[i]);
	Collision.Clear();
	CollLocalTfm.Clear();

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
	for (int i = 0; i < Collision.GetCount(); ++i)
		Collision[i]->SetTransform(CollLocalTfm[i] * Tfm);
	if (CollObj.IsValid()) CollObj->SetTransform(Tfm);
	if (Node.IsValid()) Node->SetWorldTransform(Tfm);
}
//---------------------------------------------------------------------

} // namespace Game
