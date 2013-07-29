// Loads hierarchy of scene nodes from .scn file.
// Use function declaration instead of header file where you want to call this loader.

#include <Scene/SceneNode.h>
#include <IO/BinaryReader.h>
#include <IO/Streams/FileStream.h>

namespace Scene
{

bool LoadNodesFromSCN(IO::CStream& In, PSceneNode RootNode)
{
	if (!RootNode.IsValid()) FAIL;

	IO::CBinaryReader Reader(In);

	RootNode->SetScale(Reader.Read<vector3>());
	RootNode->SetRotation(Reader.Read<quaternion>());
	RootNode->SetPosition(Reader.Read<vector3>());

	ushort Count;
	Reader.Read(Count);
	for (ushort i = 0; i < Count; ++i)
	{
		ushort DataBlockCount;
		Reader.Read(DataBlockCount);
		--DataBlockCount;

		char ClassName[256] = "Scene::C";
		n_assert(Reader.ReadString(ClassName + 8, sizeof(ClassName) - 8));

		PNodeAttribute Attr = (CNodeAttribute*)Factory->Create(CString(ClassName));

		//!!!move to Attr->LoadFromStream! some attrs may want to load not by block, but sequentially.
		//may require to read data block count inside, or ignore it for such attrs.
		for (ushort j = 0; j < DataBlockCount; ++j)
			if (!Attr->LoadDataBlock(Reader.Read<int>(), Reader)) FAIL;

		RootNode->AddAttr(*Attr);
	}

	Reader.Read(Count);
	for (ushort i = 0; i < Count; ++i)
	{
		char ChildName[256];
		n_assert(Reader.ReadString(ChildName, sizeof(ChildName)));

		//!!!REDUNDANCY! { [ { } ] } problem.
		short SMTH = Reader.Read<short>();

		if (!LoadNodesFromSCN(In, RootNode->CreateChild(CStrID(ChildName)))) FAIL;
	}

	OK;
}
//---------------------------------------------------------------------

bool LoadNodesFromSCN(const CString& FileName, PSceneNode RootNode)
{
	IO::CFileStream File;
	return File.Open(FileName, IO::SAM_READ, IO::SAP_SEQUENTIAL) &&
		LoadNodesFromSCN(File, RootNode);
}
//---------------------------------------------------------------------

}