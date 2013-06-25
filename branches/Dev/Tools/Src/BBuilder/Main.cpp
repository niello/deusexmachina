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

//!!!Export quests!

bool					ExportFromSrc;
int						Verbose = VR_ERROR;

Ptr<IO::CIOServer>		IOServer;
Ptr<Data::CDataServer>	DataServer;

nArray<nString>			FilesToPack;

nArray<nString>			CFLuaIn;
nArray<nString>			CFLuaOut;

int RunExternalToolAsProcess(CStrID Name, LPSTR pCmdLine, LPCSTR pWorkingDir = NULL);

//???pre-unwind descs on exports?
bool ProcessDesc(const nString& SrcContext, const nString& ExportContext, const nString& Name)
{
	Data::PParams Desc;
	nString ExportFilePath = ExportContext + Name + ".prm";
	if (ExportFromSrc)
	{
		IOSrv->CreateDirectory(ExportFilePath.ExtractDirName());
		Desc = DataSrv->LoadHRD(SrcContext + Name + ".hrd", false);
		DataSrv->SavePRM(ExportFilePath, Desc);
	}
	else Desc = DataSrv->LoadPRM(ExportFilePath);

	if (!Desc.IsValid()) FAIL;

	FilesToPack.Append(ExportFilePath);

	nString BaseName = Desc->Get(CStrID("_Base_"), nString::Empty);
	return BaseName.IsEmpty() || (BaseName != Name && ProcessDesc(SrcContext, ExportContext, BaseName));
}
//---------------------------------------------------------------------

//!!!???check if resources are already parsed?!
bool ProcessEntity(const Data::CParams& EntityDesc)
{
	Data::PParams Attrs;
	if (!EntityDesc.Get<Data::PParams>(Attrs, CStrID("Attrs"))) OK;

	nString AttrValue;

	if (Attrs->Get<nString>(AttrValue, CStrID("UIDesc")))
	{
		nString ExportFilePath = "Export:Game/UI/" + AttrValue + ".prm";
		if (ExportFromSrc)
		{
			IOSrv->CreateDirectory(ExportFilePath.ExtractDirName());
			DataSrv->SavePRM(ExportFilePath, DataSrv->LoadHRD("Src:Game/UI/" + AttrValue + ".hrd", false));
		}
		FilesToPack.Append(ExportFilePath);
	}

	if (Attrs->Get<nString>(AttrValue, CStrID("AIHintsDesc")))
	{
		nString ExportFilePath = "Export:Game/AI/Hints/" + AttrValue + ".prm";
		if (ExportFromSrc)
		{
			IOSrv->CreateDirectory(ExportFilePath.ExtractDirName());
			DataSrv->SavePRM(ExportFilePath, DataSrv->LoadHRD("Src:Game/AI/Hints/" + AttrValue + ".hrd", false));
		}
		FilesToPack.Append(ExportFilePath);
	}

	if (Attrs->Get<nString>(AttrValue, CStrID("SmartObjDesc")))
	{
		nString ExportFilePath = "Export:Game/AI/Smarts/" + AttrValue + ".prm";
		if (ExportFromSrc)
		{
			IOSrv->CreateDirectory(ExportFilePath.ExtractDirName());
			DataSrv->SavePRM(ExportFilePath, DataSrv->LoadHRD("Src:Game/AI/Smarts/" + AttrValue + ".hrd", false));
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

	if (Attrs->Get<nString>(AttrValue, CStrID("ActorDesc")))
		if (!ProcessDesc("Src:Game/AI/Actors/", "Export:Game/AI/Actors/", AttrValue))
		{
			n_msg(VR_ERROR, "Error processing ActorDesc '%s'\n", AttrValue.CStr());
			FAIL;
		}

	if (Attrs->Get<nString>(AttrValue, CStrID("ScriptClass")))
	{
		nString ExportFilePath = "Export:Game/ScriptClasses/" + AttrValue + ".lua";
		if (ExportFromSrc)
		{
			CFLuaIn.Append("Src:Game/ScriptClasses/" + AttrValue + ".lua");
			CFLuaOut.Append(ExportFilePath);
		}
		FilesToPack.Append(ExportFilePath);
	}

	if (Attrs->Get<nString>(AttrValue, CStrID("Script")))
	{
		nString ExportFilePath = "Export:Game/Scripts/" + AttrValue + ".lua";
		if (ExportFromSrc)
		{
			CFLuaIn.Append("Src:Game/Scripts/" + AttrValue + ".lua");
			CFLuaOut.Append(ExportFilePath);
		}
		FilesToPack.Append(ExportFilePath);
	}

	// SceneFile -> Mesh, Vars.Texture, Material, CDLODFile
	// Physics -> Shape -> FileName
	// PickShape -> FileName
	// AnimDesc -> Animation
	// Dialogue -> Script

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
			CFLuaIn.Append(SrcFilePath);
			CFLuaOut.Append(ExportFilePath);
			FilesToPack.Append(ExportFilePath); //???or after running tool?
		}
	}
	else if (IOSrv->FileExists(ExportFilePath)) FilesToPack.Append(ExportFilePath);

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

	n_printf("\n"SEP_LINE"Compiling scripts by CFLua:\n"SEP_LINE);

	if (CFLuaIn.GetCount() > 0)
	{
		n_assert(CFLuaIn.GetCount() == CFLuaOut.GetCount());

		for (int i = 0; i < CFLuaIn.GetCount(); ++i)
		{
			CFLuaIn[i] = IOSrv->ManglePath(CFLuaIn[i]);
			CFLuaOut[i] = IOSrv->ManglePath(CFLuaOut[i]);
			//!!!GetRelativePath(Base) to reduce cmd line size!
			//!!!don't forget to pass BasePath or override working directory to CFLua in that case!
		}

		nString InStr = CFLuaIn[0], OutStr = CFLuaOut[0];

		for (int i = 1; i < CFLuaIn.GetCount(); ++i)
		{
			DWORD NextLength = 32 + InStr.Length() + OutStr.Length() + CFLuaIn[i].Length() + CFLuaOut[i].Length();
			if (NextLength >= MAX_CMDLINE_CHARS)
			{
				char CmdLine[MAX_CMDLINE_CHARS];
				sprintf_s(CmdLine, "-v 5 -in %s -out %s", InStr.CStr(), OutStr.CStr());
				if (RunExternalToolAsProcess(CStrID("CFLua"), CmdLine) != 0) EXIT_APP_FAIL;
				InStr = CFLuaIn[i];
				OutStr = CFLuaOut[i];
			}
			else
			{
				InStr.Append(';');
				InStr += CFLuaIn[i];
				OutStr.Append(';');
				OutStr += CFLuaOut[i];
			}
		}

		if (InStr.FindCharIndex(' ') != INVALID_INDEX) InStr = "\"" + InStr + "\"";
		if (OutStr.FindCharIndex(' ') != INVALID_INDEX) OutStr = "\"" + OutStr + "\"";

		char CmdLine[MAX_CMDLINE_CHARS];
		sprintf_s(CmdLine, "-v 5 -in %s -out %s", InStr.CStr(), OutStr.CStr());
		if (RunExternalToolAsProcess(CStrID("CFLua"), CmdLine) != 0) EXIT_APP_FAIL;
	}

	n_printf("\n"SEP_LINE"Packing:\n"SEP_LINE);

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

	DataServer = NULL;
	IOServer = NULL;

	return NoError ? 0 : 1;
}
//---------------------------------------------------------------------
