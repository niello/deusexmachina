#include "StaticObject.h"

#include <Game/GameLevel.h>
#include <Scene/Scene.h>
#include <Physics/Collision/Shape.h>
#include <Physics/PhysicsLevel.h>
#include <Data/DataServer.h>
#include <Data/DataArray.h>

namespace Scene
{
	bool LoadNodesFromSCN(const nString& FileName, PSceneNode RootNode, bool PreloadResources = true);
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

	Desc = &ObjDesc;

	nString NodePath;
	Desc->Get<nString>(CStrID("ScenePath"), NodePath);
	nString NodeFile;
	Desc->Get<nString>(CStrID("SceneFile"), NodeFile);

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

	//!!!load collision shapes, if not scene attrs
	const nString& CompositeName = Desc->Get<nString>(CStrID("Physics"), NULL);    
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
				PShape pShape = (CShape*)Factory->Create("Physics::C" + ShapeDesc->Get<nString>(CStrID("Type")));
				pShape->Init(ShapeDesc);
				CollLocalTfm.Append(pShape->GetTransform());
				//!!!pShape->SetTransform(pShape->GetTransform() * EntityTfm);
				Collision.Append(pShape);
				Level->GetPhysics()->AttachShape(pShape);
			}
		}
	}

	if (ExistingNode)
	{
		//!!!setup collision shapes' tfm from Node->GetWorldMatrix(),
		//if needed (only if collision shapes aren't node attrs)
	}
	else SetTransform(Desc->Get<matrix44>(CStrID("Transform")));
}
//---------------------------------------------------------------------

void CStaticObject::Term()
{
	for (int i = 0; i < Collision.GetCount(); ++i)
		Level->GetPhysics()->RemoveShape(Collision[i]);
	Collision.Clear();
	CollLocalTfm.Clear();

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
	//!!!if (Node.IsValid()) Node->SetGlobalTransform(Tfm);
}
//---------------------------------------------------------------------

} // namespace Game
