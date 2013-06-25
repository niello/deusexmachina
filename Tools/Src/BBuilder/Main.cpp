#include "Main.h"

#include <IO/IOServer.h>
#include <IO/FSBrowser.h>
#include <Data/DataServer.h>
#include <Data/DataArray.h>

// Assigns used:
// Src
// Export
// Game
// UI

//???Convert from Src resources, descs or both? or load from export & warn if outdated?
//can convert from src if NO converted file, and warn if out of date!

//!!!
// Iterate over all entity templates
//   Export resources referenced by template entities

bool					ExportFromSrc;
int						Verbose = VR_ERROR;
Ptr<IO::CIOServer>		IOServer;
Ptr<Data::CDataServer>	DataServer;
nArray<nString>			FilesToPack;

int RunExternalToolAsProcess(CStrID Name, LPSTR pCmdLine);

bool ProcessEntity(const Data::CParams& EntityDesc)
{
	Data::PParams Attrs;
	if (!EntityDesc.Get<Data::PParams>(Attrs, CStrID("Attrs"))) OK;

	nString DescName;

	if (Attrs->Get<nString>(DescName, CStrID("UIDesc")))
	{
		nString ExportFilePath = "Export:Game/UI/" + DescName + ".prm";
		if (ExportFromSrc)
		{
			IOSrv->CreateDirectory(ExportFilePath.ExtractDirName());
			DataSrv->SavePRM(ExportFilePath, DataSrv->LoadHRD("Src:Game/UI/" + DescName + ".hrd", false));
		}
		FilesToPack.Append(ExportFilePath);
	}

	if (Attrs->Get<nString>(DescName, CStrID("AIHintsDesc")))
	{
		nString ExportFilePath = "Export:Game/AI/Hints/" + DescName + ".prm";
		if (ExportFromSrc)
		{
			IOSrv->CreateDirectory(ExportFilePath.ExtractDirName());
			DataSrv->SavePRM(ExportFilePath, DataSrv->LoadHRD("Src:Game/AI/Hints/" + DescName + ".hrd", false));
		}
		FilesToPack.Append(ExportFilePath);
	}

	if (Attrs->Get<nString>(DescName, CStrID("SmartObjDesc")))
	{
		nString ExportFilePath = "Export:Game/AI/Smarts/" + DescName + ".prm";
		if (ExportFromSrc)
		{
			IOSrv->CreateDirectory(ExportFilePath.ExtractDirName());
			DataSrv->SavePRM(ExportFilePath, DataSrv->LoadHRD("Src:Game/AI/Smarts/" + DescName + ".hrd", false));
		}
		FilesToPack.Append(ExportFilePath);
	}

	CStrID ItemID = Attrs->Get<CStrID>(CStrID("ItemTplID"), CStrID::Empty);
	if (ItemID.IsValid())
	{
		nString ExportFilePath = nString("Export:Game/Items/") + ItemID.CStr() + ".prm";
		if (ExportFromSrc)
		{
			IOSrv->CreateDirectory(ExportFilePath.ExtractDirName());
			DataSrv->SavePRM(ExportFilePath, DataSrv->LoadHRD(nString("Src:Game/Items/") + ItemID.CStr() + ".hrd", false));
		}
		FilesToPack.Append(ExportFilePath);
	}

	Data::PDataArray Inventory;
	if (Attrs->Get<Data::PDataArray>(Inventory, CStrID("Inventory")))
	{
		for (int i = 0; i < Inventory->GetCount(); ++i)
		{
			Data::PParams ItemDesc = Inventory->Get<Data::PParams>(i);
			ItemID = ItemDesc->Get<CStrID>(CStrID("ID"), CStrID::Empty);
			if (ItemID.IsValid())
			{
				nString ExportFilePath = nString("Export:Game/Items/") + ItemID.CStr() + ".prm";
				if (ExportFromSrc)
				{
					IOSrv->CreateDirectory(ExportFilePath.ExtractDirName());
					DataSrv->SavePRM(ExportFilePath, DataSrv->LoadHRD(nString("Src:Game/Items/") + ItemID.CStr() + ".hrd", false));
				}
				FilesToPack.Append(ExportFilePath);
			}
		}
	}

	// SceneFile -> Mesh, Vars.Texture, Material, CDLODFile
	// PickShape -> FileName
	// ScriptClass
	// Script
	// Physics -> Shape -> FileName
	// ActorDesc -> _Base_ -> _Base_ ...
	// AnimDesc -> Animation
	// Dialogue

	OK;
}
//---------------------------------------------------------------------

bool ProcessLevel(const Data::CParams& LevelDesc, const nString& Name)
{
	// Add level script

	nString ExportFilePath = "Export:Game/Levels/" + Name + ".lua";
	if (ExportFromSrc)
	{
		nString SrcFilePath = "Src:Game/Levels/" + Name + ".lua";
		if (IOSrv->FileExists(SrcFilePath))
		{
			//???or collect to batch-convert later?
			//LPCSTR Args[4] = { "-in", FileFullName.CStr(), "-out", OutFullName.CStr() };
			char CmdLine[4096];
			sprintf_s(CmdLine, "-v 0 -in %s -out %s",
				IOSrv->ManglePath(SrcFilePath).CStr(),
				IOSrv->ManglePath(ExportFilePath).CStr());
			if (RunExternalToolAsProcess(CStrID("CFLua"), CmdLine) != 0) FAIL;
		}
	}
	if (IOSrv->FileExists(ExportFilePath)) FilesToPack.Append(ExportFilePath);

	// Add navmesh (always exported, no src)

	ExportFilePath = "Export:Game/Levels/" + Name + ".nm";
	if (IOSrv->FileExists(ExportFilePath)) FilesToPack.Append(ExportFilePath);

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

	// If true, will export resources from Src to Export before packing
	ExportFromSrc = Args.GetBoolArg("-export");

	// If true, application will wait for key before exit
	bool WaitKey = Args.GetBoolArg("-waitkey");

	// Verbosity level, where 0 is silence
	Verbose = Args.GetIntArg("-v");

	// Project directory, where all content is placed. Will be a base directory for all data.
	nString ProjDir = Args.GetStringArg("-proj");
	ProjDir.ConvertBackslashes();
	ProjDir.StripTrailingSlash();

	if (ProjDir.IsEmpty()) EXIT_APP_FAIL;

	n_msg(VR_ALWAYS, SEP_LINE"BBuilder v"VERSION" for DeusExMachina engine\n(c) Vladimir \"Niello\" Orlov 2011-2013\n"SEP_LINE"\n");

	IOServer = n_new(IO::CIOServer);
	ProjDir = IOSrv->ManglePath(ProjDir);
	IOSrv->SetAssign("Proj", ProjDir);
	IOSrv->SetAssign("Src", ProjDir + "/Src");
	IOSrv->SetAssign("Export", ProjDir + "/Export");

	n_msg(VR_INFO, "Project directory: %s\n\n", ProjDir.CStr());

	DataServer = n_new(Data::CDataServer);

	// Parse levels

	nString ExportFilePath = "Export:Game/Main.prm";
	Data::PParams Desc;
	if (ExportFromSrc)
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

	FilesToPack.Append(ExportFilePath);

	IO::CFSBrowser Browser;
	if (!Browser.SetAbsolutePath(ExportFromSrc ? "Src:Game/Levels" : "Export:Game/Levels"))
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
			if (ExportFromSrc)
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

			FilesToPack.Append(ExportFilePath);

			if (!ProcessLevel(*LevelDesc, FileNoExt))
			{
				n_msg(VR_ERROR, "Error processing level '%s'\n", FileNoExt.CStr());
				continue;
			}
		}
	}
	while (Browser.NextCurrDirEntry());

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
