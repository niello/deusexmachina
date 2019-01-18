#include "SceneNodeLoaderSCN.h"

#include <Scene/SceneNode.h>
#include <IO/BinaryReader.h>
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
	Node->SetScale(Reader.Read<vector3>());
	Node->SetRotation(Reader.Read<quaternion>());
	Node->SetPosition(Reader.Read<vector3>());

	U16 Count;
	Reader.Read(Count);
	for (U16 i = 0; i < Count; ++i)
	{
		U16 DataBlockCount;
		Reader.Read(DataBlockCount);
		--DataBlockCount;

		//???!!!use FOURCC?!
		CString ClassName;
		n_assert(Reader.ReadString(ClassName));
		Scene::PNodeAttribute Attr = (Scene::CNodeAttribute*)Factory->Create(ClassName);

		//!!!move to Attr->LoadFromStream! some attrs may want to load not by block, but sequentially.
		//may require to read data block count inside, or ignore it for such attrs.
		for (U16 j = 0; j < DataBlockCount; ++j)
			if (!Attr->LoadDataBlock(Reader.Read<int>(), Reader)) FAIL;

		Node->AddAttribute(*Attr);
	}

	Reader.Read(Count);
	for (U16 i = 0; i < Count; ++i)
	{
		char ChildName[256];
		n_assert(Reader.ReadString(ChildName, sizeof(ChildName)));

		//!!!REDUNDANCY! { [ { } ] } problem.
		short SMTH = Reader.Read<short>();

		Scene::PSceneNode ChildNode = Node->CreateChild(CStrID(ChildName));
		if (ChildNode.IsNullPtr() || !LoadNode(Reader, ChildNode)) FAIL;
	}

	OK;
}
//---------------------------------------------------------------------

PResourceObject CSceneNodeLoaderSCN::CreateResource(CStrID UID)
{
	const char* pSubId;
	IO::PStream Stream = OpenStream(UID, pSubId);
	if (!Stream) return nullptr;

	IO::CBinaryReader Reader(*Stream);
	Scene::PSceneNode Root = RootNode.IsValidPtr() ? RootNode : n_new(Scene::CSceneNode);
	if (Root.IsNullPtr() || !LoadNode(Reader, Root)) return NULL;
	return Root.Get();
}
//---------------------------------------------------------------------

}