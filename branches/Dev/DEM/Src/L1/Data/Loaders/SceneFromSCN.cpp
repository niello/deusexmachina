// Loads hierarchy of scene nodes from .scn file.
// Use function declaration instead of header file where you want to call this loader.

#include <Data/BinaryReader.h>
#include <Data/Streams/FileStream.h>
#include <kernel/nkernelserver.h>
#include <scene/nscenenode.h>

namespace Load
{

bool SceneFromSCN(Data::CStream& In, nSceneNode*& Out, LPCSTR RootName, int DataBlockCount = -1)
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

				nSceneNode* pChild;
				nKernelServer::Instance()->PushCwd(Out);
				SceneFromSCN(In, pChild, ChildName, --ChildBlockCount);
				nKernelServer::Instance()->PopCwd();
			}
		}
		else if (!Out->LoadDataBlock(FourCC, Reader)) FAIL;
	}

	return !DataBlockCount || (DataBlockCount == -1 && In.IsEOF());
}
//---------------------------------------------------------------------

bool SceneFromSCN(const nString& FileName, nSceneNode*& Out, LPCSTR RootName, int DataBlockCount = -1)
{
	Data::CFileStream File;
	return File.Open(FileName, Data::SAM_READ, Data::SAP_SEQUENTIAL) &&
		SceneFromSCN(File, Out, RootName, DataBlockCount);
}
//---------------------------------------------------------------------

}
