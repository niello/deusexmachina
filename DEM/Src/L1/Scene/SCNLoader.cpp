// Loads hierarchy of scene nodes from .scn file.
// Use function declaration instead of header file where you want to call this loader.

#include <Scene/SceneNode.h>
#include <Data/BinaryReader.h>
#include <Data/Streams/FileStream.h>

namespace Scene
{

	/*
bool LoadNodesFromSCN(Data::CStream& In, PSceneNode CurrRoot)
{
	Data::CBinaryReader Reader(In);

	char ClassName[256];
	n_assert(Reader.ReadString(ClassName, sizeof(ClassName)));

	Out = (nSceneNode*)nKernelServer::Instance()->New(ClassName, RootName);
	if (!Out) FAIL;

	int FourCC;
	while (DataBlockCount && Reader.Read(FourCC))
	{
		if (DataBlockCount > 0) --DataBlockCount;

		// Load node params
		// Load attrs
		// Recurse into children

		if (FourCC == 'DLHC') // CHLD
		{
			ushort ChildCount;
			Reader.Read(ChildCount);
			for (ushort i = 0; i < ChildCount; ++i)
			{
				char ChildName[256];
				n_assert(Reader.ReadString(ChildName, sizeof(ChildName)));

				ushort ChildBlockCount;
				Reader.Read(ChildBlockCount);
				n_assert(ChildBlockCount);

				PSceneNode Child;
				LoadNodesFromSCN(In, pChild, ChildName, --ChildBlockCount);
			}
		}
		else if (!Out->LoadDataBlock(FourCC, Reader)) FAIL;
	}

	return !DataBlockCount || (DataBlockCount == -1 && In.IsEOF());
}
//---------------------------------------------------------------------

bool LoadNodesFromSCN(const nString& FileName, PSceneNode CurrRoot)
{
	Data::CFileStream File;
	return File.Open(FileName, Data::SAM_READ, Data::SAP_SEQUENTIAL) &&
		LoadNodesFromSCN(File, CurrRoot);
}
//---------------------------------------------------------------------
*/

}
