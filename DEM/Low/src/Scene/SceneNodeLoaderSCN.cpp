#include "SceneNodeLoaderSCN.h"

#include <Scene/SceneNode.h>
#include <Resources/ResourceManager.h>
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
		Scene::PNodeAttribute Attr = Core::CFactory::Instance().Create<Scene::CNodeAttribute>(ClassName);

		//!!!not all attrs may be saved with blocks! change SCN format?
		if (!Attr->LoadDataBlocks(Reader, DataBlockCount)) FAIL;

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
	if (!pResMgr) return nullptr;

	const char* pOutSubId;
	IO::PStream Stream = pResMgr->CreateResourceStream(UID, pOutSubId);
	if (!Stream || !Stream->Open(IO::SAM_READ, IO::SAP_SEQUENTIAL)) return nullptr;

	IO::CBinaryReader Reader(*Stream);
	Scene::PSceneNode Root = n_new(Scene::CSceneNode);
	if (!Root || !LoadNode(Reader, Root)) return nullptr;
	return Root;
}
//---------------------------------------------------------------------

}