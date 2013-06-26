#include "Main.h"

#include <IO/IOServer.h>
#include <IO/FSBrowser.h>
#include <IO/Streams/FileStream.h>
#include <IO/BinaryWriter.h>
#include <Data/DataServer.h>
#include <Data/DataArray.h>

bool					ExportDescs;
int						Verbose = VR_ERROR;

Ptr<IO::CIOServer>		IOServer;
Ptr<Data::CDataServer>	DataServer;
Data::PDataScheme		SceneRsrcScheme;

//!!!control duplicates on add! or sort before packing and skip dups!
// Can optimize by calculating nearest index:
//!!!if (!IsFileAdded(FileName)) FilesToPack.InsertSorted(FileName);
nArray<nString>			FilesToPack;

//!!!control duplicates! (immediately after mangle path, forex)
nArray<nString>			CFLuaIn;
nArray<nString>			CFLuaOut;

int RunExternalToolBatch(CStrID Tool, int Verb, const nArray<nString>& InList, const nArray<nString>& OutList, LPCSTR pWorkingDir = NULL);

inline bool IsFileAdded(const nString& File)
{
	return FilesToPack.BinarySearchIndex(File) != INVALID_INDEX;
}
//---------------------------------------------------------------------

bool ProcessDialogue(const nString& SrcContext, const nString& ExportContext, const nString& Name)
{
	nString ExportFilePath = ExportContext + Name + ".prm";

	if (IsFileAdded(ExportFilePath)) OK;

	Data::PParams Desc;
	if (ExportDescs)
	{
		IOSrv->CreateDirectory(ExportFilePath.ExtractDirName());
		Desc = DataSrv->LoadHRD(SrcContext + Name + ".dlg", false);
		DataSrv->SavePRM(ExportFilePath, Desc);
	}
	else Desc = DataSrv->LoadPRM(ExportFilePath);

	if (!Desc.IsValid()) FAIL;

	FilesToPack.InsertSorted(ExportFilePath);

	bool UsesScript = false;

	const Data::CDataArray& Links = *(Desc->Get<Data::PDataArray>(CStrID("Links")));
	for (int j = 0; j < Links.GetCount(); j++)
	{
		//const Data::CDataArray& Link = *(Links[j]); // Crashes vs2008 compiler :)
		const Data::CDataArray& Link = *(Links.Get(j).GetValue<Data::PDataArray>());

		if (Link.GetCount() > 2)
		{
			if (Link.Get(2).GetValue<nString>().IsValid()) UsesScript = true;
			else if (Link.GetCount() > 3 && Link.Get(3).GetValue<nString>().IsValid()) UsesScript = true;
			if (UsesScript)
			{
				int Idx = Desc->IndexOf(CStrID("ScriptFile"));
				nString ScriptFile = (Idx == INVALID_INDEX) ? Name : Desc->Get(Idx).GetValue<nString>();

				ExportFilePath = ExportContext + ScriptFile + ".lua";

				if (IsFileAdded(ExportFilePath)) break;

				if (ExportDescs)
				{
					CFLuaIn.Append(SrcContext + ScriptFile + ".lua");
					CFLuaOut.Append(ExportFilePath);
				}
				FilesToPack.InsertSorted(ExportFilePath);

				break;
			}
		}
	}

	OK;
}
//---------------------------------------------------------------------

bool ProcessCollisionShape(const nString& SrcFilePath, const nString& ExportFilePath)
{
	if (IsFileAdded(ExportFilePath)) OK;

	Data::PParams Desc;
	if (ExportDescs)
	{
		IOSrv->CreateDirectory(ExportFilePath.ExtractDirName());
		Desc = DataSrv->LoadHRD(SrcFilePath, false);
		DataSrv->SavePRM(ExportFilePath, Desc);
	}
	else Desc = DataSrv->LoadPRM(ExportFilePath);

	if (!Desc.IsValid()) FAIL;

	FilesToPack.InsertSorted(ExportFilePath);

	// Add terrain file for heightfield shapes (always exported) //???or allow building from L3DT src?
	nString FileName = Desc->Get(CStrID("FileName"), nString::Empty);
	if (!FileName.IsEmpty() && !IsFileAdded(FileName)) FilesToPack.InsertSorted(FileName);

	OK;
}
//---------------------------------------------------------------------

bool ProcessPhysicsDesc(const nString& SrcFilePath, const nString& ExportFilePath)
{
	if (IsFileAdded(ExportFilePath)) OK;

	Data::PParams Desc;
	if (ExportDescs)
	{
		IOSrv->CreateDirectory(ExportFilePath.ExtractDirName());
		Desc = DataSrv->LoadHRD(SrcFilePath, false);
		DataSrv->SavePRM(ExportFilePath, Desc);
	}
	else Desc = DataSrv->LoadPRM(ExportFilePath);

	if (!Desc.IsValid()) FAIL;

	FilesToPack.InsertSorted(ExportFilePath);

	Data::PDataArray Objects;
	if (Desc->Get(Objects, CStrID("Objects")))
	{
		for (int i = 0; i < Objects->GetCount(); ++i)
		{
			Data::PParams ObjDesc = Objects->Get<Data::PParams>(i);
			CStrID PickShape = ObjDesc->Get<CStrID>(CStrID("Shape"), CStrID::Empty);
			if (PickShape.IsValid())
				if (!ProcessCollisionShape(	nString("Src:Physics/") + PickShape.CStr() + ".hrd",
											nString("Export:Physics/") + PickShape.CStr() + ".prm"))
				{
					n_msg(VR_ERROR, "Error processing collision shape '%s'\n", PickShape.CStr());
					FAIL;
				}
		}
	}


	OK;
}
//---------------------------------------------------------------------

bool ProcessAnimDesc(const nString& SrcFilePath, const nString& ExportFilePath)
{
	if (IsFileAdded(ExportFilePath)) OK;

	Data::PParams Desc;
	if (ExportDescs)
	{
		IOSrv->CreateDirectory(ExportFilePath.ExtractDirName());
		Desc = DataSrv->LoadHRD(SrcFilePath, false);
		DataSrv->SavePRM(ExportFilePath, Desc);
	}
	else Desc = DataSrv->LoadPRM(ExportFilePath);

	if (!Desc.IsValid()) FAIL;

	FilesToPack.InsertSorted(ExportFilePath);

	for (int i = 0; i < Desc->GetCount(); ++i)
	{
		//???!!!allow compile or batch-compile?
		// can add Model resource description, associated with this anim, to CFModel list
		nString FileName = nString("Export:Anim/") + Desc->Get(i).GetValue<CStrID>().CStr();
		if (!IsFileAdded(FileName)) FilesToPack.InsertSorted(FileName);
	}

	OK;
}
//---------------------------------------------------------------------

//???pre-unwind descs on exports?
bool ProcessDescWithParents(const nString& SrcContext, const nString& ExportContext, const nString& Name)
{
	nString ExportFilePath = ExportContext + Name + ".prm";

	if (IsFileAdded(ExportFilePath)) OK;

	Data::PParams Desc;
	if (ExportDescs)
	{
		IOSrv->CreateDirectory(ExportFilePath.ExtractDirName());
		Desc = DataSrv->LoadHRD(SrcContext + Name + ".hrd", false);
		DataSrv->SavePRM(ExportFilePath, Desc);
	}
	else Desc = DataSrv->LoadPRM(ExportFilePath);

	if (!Desc.IsValid()) FAIL;

	FilesToPack.InsertSorted(ExportFilePath);

	nString BaseName = Desc->Get(CStrID("_Base_"), nString::Empty);
	return BaseName.IsEmpty() || (BaseName != Name && ProcessDescWithParents(SrcContext, ExportContext, BaseName));
}
//---------------------------------------------------------------------

bool ProcessMaterialDesc(const nString& Name)
{
	nString ExportFilePath = "Export:Materials/" + Name + ".prm";

	if (IsFileAdded(ExportFilePath)) OK;

	Data::PParams Desc;
	if (ExportDescs)
	{
		IOSrv->CreateDirectory(ExportFilePath.ExtractDirName());
		Desc = DataSrv->LoadHRD("Src:Materials/" + Name + ".hrd", false);
		DataSrv->SavePRM(ExportFilePath, Desc);
	}
	else Desc = DataSrv->LoadPRM(ExportFilePath);

	if (!Desc.IsValid()) FAIL;

	FilesToPack.InsertSorted(ExportFilePath);

	Data::PParams Textures;
	if (Desc->Get(Textures, CStrID("Textures")))
	{
		//!!!when export from src, find resource desc and add source texture to CFTexture list!
		for (int i = 0; i < Textures->GetCount(); ++i)
		{
			nString FileName = nString("Export:Textures/") + Textures->Get(i).GetValue<CStrID>().CStr();
			if (!IsFileAdded(FileName)) FilesToPack.InsertSorted(FileName);
		}
	}

	OK;
}
//---------------------------------------------------------------------

bool ProcessSceneNodeRefs(const Data::CParams& NodeDesc)
{
	Data::PDataArray Attrs;
	if (NodeDesc.Get(Attrs, CStrID("Attrs")))
	{
		for (int i = 0; i < Attrs->GetCount(); ++i)
		{
			Data::PParams AttrDesc = Attrs->Get<Data::PParams>(i);

			Data::PParams Textures;
			if (AttrDesc->Get(Textures, CStrID("Textures")))
			{
				//!!!when export from src, find resource desc and add source texture to CFTexture list!
				for (int i = 0; i < Textures->GetCount(); ++i)
				{
					nString FileName = nString("Export:Textures/") + Textures->Get(i).GetValue<CStrID>().CStr();
					if (!IsFileAdded(FileName)) FilesToPack.InsertSorted(FileName);
				}
			}

			Data::CData* pValue;

			if (AttrDesc->Get(pValue, CStrID("Material")))
				if (!ProcessMaterialDesc(pValue->GetValue<CStrID>().CStr())) FAIL;

			//!!!when export from src, find resource desc and add source Model to CFModel list!
			if (AttrDesc->Get(pValue, CStrID("Mesh")))
			{
				nString FileName = nString("Export:Meshes/") + pValue->GetValue<CStrID>().CStr() + ".nvx2"; //!!!change extension!
				if (!IsFileAdded(FileName)) FilesToPack.InsertSorted(FileName);
			}

			//!!!when export from src, find resource desc and add source BT to CFTerrain list!
			if (AttrDesc->Get(pValue, CStrID("CDLODFile")))
			{
				nString FileName = nString("Export:Terrain/") + pValue->GetValue<CStrID>().CStr() + ".cdlod";
				if (!IsFileAdded(FileName)) FilesToPack.InsertSorted(FileName);
			}
		}
	}

	Data::PParams Children;
	if (NodeDesc.Get(Children, CStrID("Children")))
		for (int i = 0; i < Children->GetCount(); ++i)
			if (!ProcessSceneNodeRefs(*Children->Get(i).GetValue<Data::PParams>())) FAIL;

	OK;
}
//---------------------------------------------------------------------

// Always processed from Src
bool ProcessSceneResource(const nString& SrcFilePath, const nString& ExportFilePath)
{
	if (IsFileAdded(ExportFilePath)) OK;

	Data::PParams Desc = DataSrv->LoadHRD(SrcFilePath, false);
	if (!Desc.IsValid()) FAIL;

	if (ExportDescs)
	{
		IOSrv->CreateDirectory(ExportFilePath.ExtractDirName());
		IO::CFileStream File;
		if (!File.Open(ExportFilePath, IO::SAM_WRITE)) FAIL;
		IO::CBinaryWriter Writer(File);
		Writer.WriteParams(*Desc, *SceneRsrcScheme);
		File.Close();
	}

	FilesToPack.InsertSorted(ExportFilePath);

	return ProcessSceneNodeRefs(*Desc);
}
//---------------------------------------------------------------------

bool ProcessDesc(const nString& SrcFilePath, const nString& ExportFilePath)
{
	if (IsFileAdded(ExportFilePath)) OK;

	if (ExportDescs)
	{
		IOSrv->CreateDirectory(ExportFilePath.ExtractDirName());
		if (!DataSrv->SavePRM(ExportFilePath, DataSrv->LoadHRD(SrcFilePath, false))) FAIL;
	}
	FilesToPack.InsertSorted(ExportFilePath);

	OK;
}
//---------------------------------------------------------------------

bool ProcessEntity(const Data::CParams& EntityDesc)
{
	Data::PParams Attrs;
	if (!EntityDesc.Get<Data::PParams>(Attrs, CStrID("Attrs"))) OK;

	nString AttrValue;

	if (Attrs->Get<nString>(AttrValue, CStrID("UIDesc")))
		if (!ProcessDesc("Src:Game/UI/" + AttrValue + ".hrd", "Export:Game/UI/" + AttrValue + ".prm"))
		{
			n_msg(VR_ERROR, "Error processing UI desc '%s'\n", AttrValue.CStr());
			FAIL;
		}

	if (Attrs->Get<nString>(AttrValue, CStrID("AIHintsDesc")))
		if (!ProcessDesc("Src:Game/AI/Hints/" + AttrValue + ".hrd", "Export:Game/AI/Hints/" + AttrValue + ".prm"))
		{
			n_msg(VR_ERROR, "Error processing AI hints desc '%s'\n", AttrValue.CStr());
			FAIL;
		}

	if (Attrs->Get<nString>(AttrValue, CStrID("SmartObjDesc")))
		if (!ProcessDesc("Src:Game/AI/Smarts/" + AttrValue + ".hrd", "Export:Game/AI/Smarts/" + AttrValue + ".prm"))
		{
			n_msg(VR_ERROR, "Error processing AI smart object desc '%s'\n", AttrValue.CStr());
			FAIL;
		}

	CStrID ItemID = Attrs->Get<CStrID>(CStrID("ItemTplID"), CStrID::Empty);
	if (ItemID.IsValid())
		if (!ProcessDesc(nString("Src:Game/Items/") + ItemID.CStr() + ".hrd", nString("Export:Game/Items/") + ItemID.CStr() + ".prm"))
		{
			n_msg(VR_ERROR, "Error processing item desc '%s'\n", AttrValue.CStr());
			FAIL;
		}

	Data::PDataArray Inventory;
	if (Attrs->Get<Data::PDataArray>(Inventory, CStrID("Inventory")))
	{
		for (int i = 0; i < Inventory->GetCount(); ++i)
		{
			ItemID = Inventory->Get<Data::PParams>(i)->Get<CStrID>(CStrID("ID"), CStrID::Empty);
			if (ItemID.IsValid() &&
				!ProcessDesc(	nString("Src:Game/Items/") + ItemID.CStr() + ".hrd",
								nString("Export:Game/Items/") + ItemID.CStr() + ".prm"))
			{
				n_msg(VR_ERROR, "Error processing item desc '%s'\n", AttrValue.CStr());
				FAIL;
			}
		}
	}

	if (Attrs->Get<nString>(AttrValue, CStrID("ActorDesc")))
		if (!ProcessDescWithParents("Src:Game/AI/Actors/", "Export:Game/AI/Actors/", AttrValue))
		{
			n_msg(VR_ERROR, "Error processing AI actor desc '%s'\n", AttrValue.CStr());
			FAIL;
		}

	if (Attrs->Get<nString>(AttrValue, CStrID("ScriptClass")))
	{
		nString ExportFilePath = "Export:Game/ScriptClasses/" + AttrValue + ".lua";
		if (!IsFileAdded(ExportFilePath))
		{
			if (ExportDescs)
			{
				CFLuaIn.Append("Src:Game/ScriptClasses/" + AttrValue + ".lua");
				CFLuaOut.Append(ExportFilePath);
			}
			FilesToPack.InsertSorted(ExportFilePath);
		}
	}

	if (Attrs->Get<nString>(AttrValue, CStrID("Script")))
	{
		nString ExportFilePath = "Export:Game/Scripts/" + AttrValue + ".lua";
		if (!IsFileAdded(ExportFilePath))
		{
			if (ExportDescs)
			{
				CFLuaIn.Append("Src:Game/Scripts/" + AttrValue + ".lua");
				CFLuaOut.Append(ExportFilePath);
			}
			FilesToPack.InsertSorted(ExportFilePath);
		}
	}

	CStrID PickShape = Attrs->Get<CStrID>(CStrID("PickShape"), CStrID::Empty);
	if (PickShape.IsValid())
		if (!ProcessCollisionShape(	nString("Src:Physics/") + PickShape.CStr() + ".hrd",
									nString("Export:Physics/") + PickShape.CStr() + ".prm"))
		{
			n_msg(VR_ERROR, "Error processing collision shape '%s'\n", PickShape.CStr());
			FAIL;
		}

	if (Attrs->Get<nString>(AttrValue, CStrID("Physics")))
		if (!ProcessPhysicsDesc(nString("Src:Physics/") + AttrValue + ".hrd",
								nString("Export:Physics/") + AttrValue + ".prm"))
		{
			n_msg(VR_ERROR, "Error processing physics desc '%s'\n", AttrValue.CStr());
			FAIL;
		}

	if (Attrs->Get<nString>(AttrValue, CStrID("AnimDesc")))
		if (!ProcessAnimDesc(	nString("Src:Game/Anim/") + AttrValue + ".hrd",
								nString("Export:Game/Anim/") + AttrValue + ".prm"))
		{
			n_msg(VR_ERROR, "Error processing animation desc '%s'\n", AttrValue.CStr());
			FAIL;
		}

	if (Attrs->Get<nString>(AttrValue, CStrID("Dialogue")))
		if (!ProcessDialogue("Src:Game/Dlg/", "Export:Game/Dlg/", AttrValue))
		{
			n_msg(VR_ERROR, "Error processing dialogue desc '%s'\n", AttrValue.CStr());
			FAIL;
		}

	if (Attrs->Get<nString>(AttrValue, CStrID("SceneFile")))
		if (!ProcessSceneResource("Src:Scene/" + AttrValue + ".hrd", "Export:Scene/" + AttrValue + ".scn"))
		{
			n_msg(VR_ERROR, "Error processing scene resource '%s'\n", AttrValue.CStr());
			FAIL;
		}

	OK;
}
//---------------------------------------------------------------------

bool ProcessLevel(const Data::CParams& LevelDesc, const nString& Name)
{
	// Add level script

	nString ExportFilePath = "Export:Game/Levels/" + Name + ".lua";
	if (ExportDescs)
	{
		nString SrcFilePath = "Src:Game/Levels/" + Name + ".lua";
		if (!IsFileAdded(SrcFilePath) && IOSrv->FileExists(SrcFilePath))
		{
			CFLuaIn.Append(SrcFilePath);
			CFLuaOut.Append(ExportFilePath);
			FilesToPack.InsertSorted(ExportFilePath); //???or after running tool?
		}
	}
	else if (!IsFileAdded(ExportFilePath) && IOSrv->FileExists(ExportFilePath)) FilesToPack.InsertSorted(ExportFilePath);

	// Add navmesh (always exported, no src)

	ExportFilePath = "Export:Game/Levels/" + Name + ".nm";
	if (!IsFileAdded(ExportFilePath) && IOSrv->FileExists(ExportFilePath)) FilesToPack.InsertSorted(ExportFilePath);

	// Export entities

	Data::PParams SubDesc;
	if (LevelDesc.Get(SubDesc, CStrID("Entities")))
	{
		for (int i = 0; i < SubDesc->GetCount(); ++i)
		{
			const Data::CParam& EntityPrm = SubDesc->Get(i);
			if (!EntityPrm.IsA<Data::PParams>()) continue;
			Data::PParams EntityDesc = EntityPrm.GetValue<Data::PParams>();
			n_msg(VR_INFO, "Processing entity '%s'...\n", EntityPrm.GetName().CStr());
			if (!EntityDesc.IsValid() || !ProcessEntity(*EntityDesc))
			{
				n_msg(VR_ERROR, "Error processing entity '%s'\n", EntityPrm.GetName().CStr());
				FAIL;
			}
		}
	}

	OK;
}
//---------------------------------------------------------------------

int main(int argc, const char** argv)
{
	nCmdLineArgs Args(argc, argv);

	// If true, will re-export descs (not resources!) from Src to Export before packing
	ExportDescs = Args.GetBoolArg("-export");
	//!!!also can store ExportResources or smth, because they are more heavy than descs!

	// If true, application will wait for key before exit
	bool WaitKey = Args.GetBoolArg("-waitkey");

	// Verbosity level, where 0 is silence
	Verbose = Args.GetIntArg("-v");

	// Verbosity level for external tools
	int ExtVerb = Args.GetIntArg("-ev");

	// Project directory, where all content is placed. Will be a base directory for all data.
	nString ProjDir = Args.GetStringArg("-proj");
	ProjDir.ConvertBackslashes();
	ProjDir.StripTrailingSlash();
	if (ProjDir.IsEmpty()) EXIT_APP_FAIL;

	// Build directory, to where final data will be saved.
	nString BuildDir = Args.GetStringArg("-build");
	BuildDir.ConvertBackslashes();
	BuildDir.StripTrailingSlash();
	if (BuildDir.IsEmpty()) EXIT_APP_FAIL;

	n_msg(VR_ALWAYS, SEP_LINE"BBuilder v"VERSION" for DeusExMachina engine\n(c) Vladimir \"Niello\" Orlov 2011-2013\n"SEP_LINE"\n");

	IOServer = n_new(IO::CIOServer);
	ProjDir = IOSrv->ManglePath(ProjDir);
	BuildDir = IOSrv->ManglePath(BuildDir);
	IOSrv->SetAssign("Proj", ProjDir);
	IOSrv->SetAssign("Build", BuildDir);
	IOSrv->SetAssign("Src", ProjDir + "/Src");
	IOSrv->SetAssign("Export", ProjDir + "/Export");

	n_msg(VR_INFO, "Project directory: %s\nBuild directory: %s\n", ProjDir.CStr(), BuildDir.CStr());

	DataServer = n_new(Data::CDataServer);

	if (!DataSrv->LoadDataSchemes("home:DataSchemes/SceneNodes.dss"))
	{
		n_msg(VR_ERROR, "BBuilder: Failed to read 'home:DataSchemes/SceneNodes.dss'");
		EXIT_APP_FAIL;
	}

	SceneRsrcScheme = DataSrv->GetDataScheme(CStrID("SceneNode"));

	// Process levels

	n_printf("\n"SEP_LINE"Processing levels and entities:\n"SEP_LINE);

	nString ExportFilePath = "Export:Game/Main.prm";
	Data::PParams Desc;
	if (ExportDescs)
	{
		IOSrv->CreateDirectory("Export:Game/Levels");
		Desc = DataSrv->LoadHRD("Src:Game/Main.hrd", false);
		DataSrv->SavePRM(ExportFilePath, Desc);
	}
	else Desc = DataSrv->LoadPRM(ExportFilePath, false);

	if (!Desc.IsValid())
	{
		n_msg(VR_ERROR, "Error loading main game desc\n");
		EXIT_APP_FAIL;
	}

	FilesToPack.InsertSorted(ExportFilePath);

	IO::CFSBrowser Browser;
	if (!Browser.SetAbsolutePath(ExportDescs ? "Src:Game/Levels" : "Export:Game/Levels"))
	{
		n_msg(VR_ERROR, "Could not open directory '%s' for reading!\n", Browser.GetCurrentPath().CStr());
		EXIT_APP_FAIL;
	}

	if (!Browser.IsCurrDirEmpty()) do
	{
		if (Browser.IsCurrEntryFile())
		{
			if (!Browser.GetCurrEntryName().CheckExtension("hrd")) continue;

			nString FileNoExt = Browser.GetCurrEntryName();
			FileNoExt.StripExtension();
			n_msg(VR_INFO, "Processing level '%s'...\n", FileNoExt.CStr());

			ExportFilePath = "Export:Game/Levels/" + FileNoExt + ".prm";
			Data::PParams LevelDesc;
			if (ExportDescs)
			{
				LevelDesc = DataSrv->LoadHRD("Src:Game/Levels/" + Browser.GetCurrEntryName(), false);
				DataSrv->SavePRM(ExportFilePath, LevelDesc);
			}
			else LevelDesc = DataSrv->LoadPRM(ExportFilePath, false);

			if (!LevelDesc.IsValid())
			{
				n_msg(VR_ERROR, "Error loading level '%s' desc\n", FileNoExt.CStr());
				continue;
			}

			FilesToPack.InsertSorted(ExportFilePath);

			if (!ProcessLevel(*LevelDesc, FileNoExt))
			{
				n_msg(VR_ERROR, "Error processing level '%s'\n", FileNoExt.CStr());
				continue;
			}
		}
	}
	while (Browser.NextCurrDirEntry());

	n_printf("\n"SEP_LINE"Processing entity templates:\n"SEP_LINE"!!!NOT IMPLEMENTED!!!\n");

//!!!Export ALL entity templates!

	// Add AI tables
	// Add quests and task scripts
	// Add system resources
	// Add the whole CEGUI directory (as directory, but mb to the same array)
	// Convert frame shaders
	// Compile all shaders of all frame shaders

	n_printf("\n"SEP_LINE"Compiling scripts by CFLua:\n"SEP_LINE);

	if (RunExternalToolBatch(CStrID("CFLua"), ExtVerb, CFLuaIn, CFLuaOut) != 0) EXIT_APP_FAIL;

	n_printf("\n"SEP_LINE"Packing:\n"SEP_LINE);

	//if (!PackFiles(FilesToPack, "Build:Export.npk")) EXIT_APP_FAIL;

	// All -> mangle, get full path, get Export assign mangled full path, check if inside, pack
	// All not inside -> report resources not packed
	//DWORD WINAPI GetFullPathName(
	//  __in          LPCTSTR lpFileName,
	//  __in          DWORD nBufferLength,
	//  __out         LPTSTR lpBuffer,
	//  __out         LPTSTR* lpFilePart
	//);

	//sort
	//remove/ignore duplicates

	for (int i = 0; i < FilesToPack.GetCount(); ++i)
		n_printf("%s\n", FilesToPack[i].CStr());

	EXIT_APP_OK;
}
//---------------------------------------------------------------------

int ExitApp(bool NoError, bool WaitKey)
{
	if (!NoError) n_msg(VR_ERROR, "Building aborted due to errors.\n");
	if (WaitKey)
	{
		n_printf("\nPress any key to exit...\n");
		getch();
	}

	SceneRsrcScheme = NULL;
	DataServer = NULL;
	IOServer = NULL;

	return NoError ? 0 : 1;
}
//---------------------------------------------------------------------
