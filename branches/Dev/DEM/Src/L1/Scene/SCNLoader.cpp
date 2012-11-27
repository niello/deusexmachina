// Loads hierarchy of scene nodes from .scn file.
// Use function declaration instead of header file where you want to call this loader.

#include <Scene/SceneNode.h>
#include <Data/BinaryReader.h>
#include <Data/Streams/FileStream.h>

namespace Scene
{

bool LoadNodesFromSCN(Data::CStream& In, PSceneNode RootNode)
{
	if (!RootNode.isvalid()) FAIL;

	Data::CBinaryReader Reader(In);

	int FourCC;
	while (Reader.Read(FourCC))
	{
		switch (FourCC)
		{
			case 'RTTA': // ATTR
			{
				char ClassName[256];
				n_assert(Reader.ReadString(ClassName, sizeof(ClassName)));
				static const nString StrAttr("Scene::C");

				PSceneNodeAttr Attr = (CSceneNodeAttr*)CoreFct->Create(StrAttr + ClassName);
				RootNode->AddAttr(*Attr);

				// Load all attr data blocks!
				//if (!Out->LoadDataBlock(FourCC, Reader)) FAIL;

				break;
			}
			case 'DLHC': // CHLD
			{
				ushort ChildCount;
				Reader.Read(ChildCount);
				for (ushort i = 0; i < ChildCount; ++i)
				{
					char ChildName[256];
					n_assert(Reader.ReadString(ChildName, sizeof(ChildName)));
					LoadNodesFromSCN(In, RootNode->CreateChild(CStrID(ChildName)));
				}
				break;
			}
			case 'LCSS': // SSCL
			{
				RootNode->SetScale(Reader.Read<vector3>());
				break;
			}
			case 'TORS': // SROT
			{
				RootNode->SetRotation(Reader.Read<quaternion>());
				break;
			}
			case 'SOPS': // SPOS
			{
				RootNode->SetPosition(Reader.Read<vector3>());
				break;
			}
			default: n_error("Unknown scene node data tag!");
		}
	}

	OK;
}
//---------------------------------------------------------------------

bool LoadNodesFromSCN(const nString& FileName, PSceneNode RootNode)
{
	Data::CFileStream File;
	return File.Open(FileName, Data::SAM_READ, Data::SAP_SEQUENTIAL) &&
		LoadNodesFromSCN(File, RootNode);
}
//---------------------------------------------------------------------

}
