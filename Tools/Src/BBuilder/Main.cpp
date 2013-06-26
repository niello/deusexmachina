#include "Main.h"

#include <IO/IOServer.h>
#include <IO/FSBrowser.h>
#include <Data/DataServer.h>

bool					ExportDescs;
int						Verbose = VR_ERROR;

Ptr<IO::CIOServer>		IOServer;
Ptr<Data::CDataServer>	DataServer;

//!!!control duplicates on add! or sort before packing and skip dups!
// Can optimize by calculating nearest index:
//!!!if (!IsFileAdded(FileName)) FilesToPack.InsertSorted(FileName);
nArray<nString>			FilesToPack;

//!!!control duplicates! (immediately after mangle path, forex)
nArray<nString>			CFLuaIn;
nArray<nString>			CFLuaOut;

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

	n_msg(VR_ALWAYS, SEP_LINE TOOL_NAME" v"VERSION" for DeusExMachina engine\n(c) Vladimir \"Niello\" Orlov 2011-2013\n"SEP_LINE"\n");

	IOServer = n_new(IO::CIOServer);
	ProjDir = IOSrv->ManglePath(ProjDir);
	BuildDir = IOSrv->ManglePath(BuildDir);
	IOSrv->SetAssign("Proj", ProjDir);
	IOSrv->SetAssign("Build", BuildDir);
	IOSrv->SetAssign("Src", ProjDir + "/Src");
	IOSrv->SetAssign("Export", ProjDir + "/Export");

	////!!!read setup file with assigns!
	//DataSrv->SetAssign("shaders", "home:shaders");
	//DataSrv->SetAssign("renderpath", "home:shaders");
	//DataSrv->SetAssign("export", Export);
	//DataSrv->SetAssign("src", Proj + "/src");
	//DataSrv->SetAssign("scene", Export + "/Scene");
	//DataSrv->SetAssign("scenesrc", "src:Scene");
	//DataSrv->SetAssign("dlg", Export + "/game/dlg");
	//DataSrv->SetAssign("dlgsrc", "src:game/dlg");
	//DataSrv->SetAssign("physics", Export + "/physics");
	//DataSrv->SetAssign("meshes", Export + "/meshes");
	//DataSrv->SetAssign("materials", Export + "/materials");
	//DataSrv->SetAssign("mtlsrc", Export + "/materials");
	//DataSrv->SetAssign("textures", Export + "/textures");
	//DataSrv->SetAssign("anims", Export + "/anims");

	n_msg(VR_INFO, "Project directory: %s\nBuild directory: %s\n", ProjDir.CStr(), BuildDir.CStr());

	DataServer = n_new(Data::CDataServer);

	if (!DataSrv->LoadDataSchemes("home:DataSchemes/SceneNodes.dss"))
	{
		n_msg(VR_ERROR, "BBuilder: Failed to read 'home:DataSchemes/SceneNodes.dss'");
		EXIT_APP_FAIL;
	}

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

	////???!!!or parse frame shader vars?!
	//AddRsrcIfUnique("Export:Textures/System/Noise.dds", ResourceFiles, "Texture");
	//AddRsrcIfUnique("Export:Textures/System/White.dds", ResourceFiles, "Texture");
	//AddRsrcIfUnique("Export:Textures/System/NoBump.dds", ResourceFiles, "Texture");
	//if (!CopyDirectoryToBuild("Proj:Project/Input/", "Build:Data/Input/")) goto error;
	//if (!CopyDirectoryToBuild("Proj:Project/Shaders/", "Build:Data/Shaders/")) goto error;

	n_printf("\n"SEP_LINE"Compiling scripts by CFLua:\n"SEP_LINE);

	if (RunExternalToolBatch(CStrID("CFLua"), ExtVerb, CFLuaIn, CFLuaOut) != 0) EXIT_APP_FAIL;

	n_printf("\n"SEP_LINE"Packing:\n"SEP_LINE);

	for (int i = 0; i < FilesToPack.GetCount(); ++i)
	{
		 FilesToPack[i] = IOSrv->ManglePath(FilesToPack[i]);
		 FilesToPack[i].ToLower();
	}
	FilesToPack.Sort();

	nString DestFile = "Build:Export.npk";
	if (PackFiles(FilesToPack, DestFile, ProjDir, "Export"))
	{
		n_msg(VR_INFO, "\nNPK file:      %s\nNPK file size: %.3f MB\n",
			IOSrv->ManglePath(DestFile).CStr(),
			IOSrv->GetFileSize(DestFile) / (1024.f * 1024.f));
	}
	else
	{
		n_msg(VR_ERROR, "ERROR IN FILE GENERATION, DELETING NPK FILE\n");
		IOSrv->DeleteFile(DestFile);
		EXIT_APP_FAIL;
	}

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

	DataServer = NULL;
	IOServer = NULL;

	return NoError ? 0 : 1;
}
//---------------------------------------------------------------------

//!!!DBG TMP! Door KFA animation
	/*{
		Data::CFileStream File;
		if (File.Open("anims:examples/Door.kfa", Data::SAM_WRITE))
		{
			Data::CBinaryWriter Wr(File);
			Wr.Write('KFAN');
			Wr.Write(0.5f);
			Wr.Write<DWORD>(1);
			Wr.Write<LPCSTR>("DoorBody");
			Wr.Write((int)Anim::Chnl_Rotation);
			Wr.Write<DWORD>(2);
			quaternion q;
			Wr.Write(q);
			Wr.Write(0.f);
			q.set_rotate_y(PI / 2.f);
			Wr.Write(q);
			Wr.Write(0.5f);
			File.Close();
		}
	}*/
