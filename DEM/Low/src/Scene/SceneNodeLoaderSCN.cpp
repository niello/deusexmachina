#include "SceneNodeLoaderSCN.h"
#include <Scene/SceneNode.h>
#include <Scene/NodeAttribute.h>
#include <Resources/ResourceManager.h>
#include <IO/BinaryReader.h>
#include <Math/SIMDMath.h>
#include <Core/Factory.h>

namespace Resources
{

const Core::CRTTI& CSceneNodeLoaderSCN::GetResultType() const
{
	return Scene::CSceneNode::RTTI;
}
//---------------------------------------------------------------------

bool LoadNode(IO::CBinaryReader& Reader, Scene::PSceneNode Node)
{
	Node->SetLocalScale(Math::ToSIMD(Reader.Read<vector3>()));
	Node->SetLocalRotation(Math::ToSIMD(Reader.Read<quaternion>()));
	Node->SetLocalPosition(Math::ToSIMD(Reader.Read<vector3>()));

	U16 Count;
	Reader.Read(Count);
	for (U16 i = 0; i < Count; ++i)
	{
		U16 DataBlockCount;
		Reader.Read(DataBlockCount);
		--DataBlockCount;

		U32 ClassFourCC;
		n_verify(Reader.Read(ClassFourCC));
		Scene::PNodeAttribute Attr = Core::CFactory::Instance().Create<Scene::CNodeAttribute>(ClassFourCC);

		//!!!not all attrs may be saved with blocks! change SCN format?
		if (!Attr || !Attr->LoadDataBlocks(Reader, DataBlockCount)) FAIL;

		Node->AddAttribute(*Attr);
	}

	Reader.Read(Count);
	for (U16 i = 0; i < Count; ++i)
	{
		CStrID ChildID;
		n_verify(Reader.Read(ChildID));

		//!!!FIXME: REDUNDANCY! { [ { } ] } problem.
		Reader.Read<short>();

		Scene::PSceneNode ChildNode = Node->CreateChild(ChildID);
		if (!ChildNode || !LoadNode(Reader, ChildNode)) FAIL;
	}

	OK;
}
//---------------------------------------------------------------------

Core::PObject CSceneNodeLoaderSCN::CreateResource(CStrID UID)
{
	const char* pOutSubId;
	IO::PStream Stream = _ResMgr.CreateResourceStream(UID.CStr(), pOutSubId, IO::SAP_SEQUENTIAL);
	if (!Stream || !Stream->IsOpened()) return nullptr;

	IO::CBinaryReader Reader(*Stream);
	Scene::PSceneNode Root = n_new(Scene::CSceneNode);
	if (!Root || !LoadNode(Reader, Root)) return nullptr;
	return Root;
}
//---------------------------------------------------------------------

}
