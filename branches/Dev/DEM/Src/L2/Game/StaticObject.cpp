#include "StaticObject.h"

#include <Game/GameLevel.h>
#include <Scene/Scene.h>
#include <Physics/Collision/Shape.h>
#include <Physics/CollisionObject.h>
#include <Physics/PhysicsWorldOld.h>
#include <Physics/PhysicsWorld.h>
#include <Physics/PhysicsServer.h>
#include <Data/DataServer.h>
#include <Data/DataArray.h>

namespace Scene
{
	bool LoadNodesFromSCN(const nString& FileName, PSceneNode RootNode, bool PreloadResources = true);
}

namespace Physics
{
	PCollisionShape LoadCollisionShapeFromPRM(CStrID UID, const nString& FileName);
}

namespace Game
{
__ImplementClassNoFactory(Game::CStaticObject, Core::CRefCounted);

using namespace Physics;

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
				PShape pShape = (CShape*)Factory->Create("Physics::C" + ShCls);
				pShape->Init(ShapeDesc);
				CollLocalTfm.Append(pShape->GetTransform());
				Collision.Append(pShape);
				Level->GetPhysicsOld()->AttachShape(pShape);
				//???associate collision shape with game entity UID?
			}
		}
	}

	if (ExistingNode)
	{
		for (int i = 0; i < Collision.GetCount(); ++i)
			Collision[i]->SetTransform(CollLocalTfm[i] * Node->GetWorldMatrix());
	}
	else SetTransform(Desc->Get<matrix44>(CStrID("Transform")));

	const matrix44& Tfm = Node.IsValid() ? Node->GetWorldMatrix() : Desc->Get<matrix44>(CStrID("Transform"));

	//???use composites in the new physics system?
	//call it CCompoundBody
	//composite is a set of bodies and joints
	//each body can/must have one shape
	//shape can be compound
	//composite can't have non-body collision shapes
	//body or compound body can be mapped as controllers to the scene hierarchy
	CStrID Coll = Desc->Get<CStrID>(CStrID("Collision"), CStrID::Empty);    
	if (Coll.IsValid() && Level->GetPhysics())
	{
		Physics::PCollisionShape Shape = PhysicsSrv->CollShapeMgr.GetTypedResource(Coll);
		if (!Shape.IsValid())
			Shape = LoadCollisionShapeFromPRM(Coll, nString("physics:") + Coll.CStr() + ".hrd"); //!!!prm!
		//!!!???what if shape is found but is not loaded? RESMGR problem!
		n_assert(Shape->IsLoaded());

		CollObj = n_new(Physics::CCollisionObject)(*Shape);

		vector3 Offset;
		if (Shape->GetOffset(Offset))
		{
			matrix44 OffsetTfm(Tfm);
			OffsetTfm.translate(Offset);
			Level->GetPhysics()->AddCollisionObject(*CollObj, OffsetTfm, 0x01, 0xffff); //!!!set normal flags!
		}
		else Level->GetPhysics()->AddCollisionObject(*CollObj, Tfm, 0x01, 0xffff); //!!!set normal flags!
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
		Level->GetPhysics()->RemoveCollisionObject(*CollObj);
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
