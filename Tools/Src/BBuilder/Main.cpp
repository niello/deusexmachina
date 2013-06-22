#include "Main.h"

#include <IO/IOServer.h>
#include <IO/FSBrowser.h>
#include <Data/DataServer.h>

// Find game file. If use Src, export it here, else ?update from Src or just notify the Src is changed?
// Iterate over all level files. If use Src, export it here, else ?update from Src or just notify the Src is changed?
//   Export resources referenced by level entities
// Iterate over all entity templates
//   Export resources referenced by template entities
//

int						Verbose = VR_ERROR;
Ptr<IO::CIOServer>		IOServer;
Ptr<Data::CDataServer>	DataServer;
nArray<nString>			FilesToPack;

int RunExternalToolAsProcess(CStrID Name, LPSTR pCmdLine);

int main(int argc, const char** argv)
{
	nCmdLineArgs Args(argc, argv);

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

	IO::CFSBrowser Browser;
	if (!Browser.SetAbsolutePath("Src:Game/Levels"))
	{
		n_msg(VR_ERROR, "Could not open directory 'Src:Game/Levels' for reading!\n");
		EXIT_APP_FAIL;
	}

	IOSrv->CreateDirectory("Export:Game/Levels");

	Data::PParams Desc = DataSrv->LoadHRD("Src:Game/Main.hrd", false);
	if (!Desc.IsValid())
	{
		n_msg(VR_ERROR, "Error loading main game desc...\n");
		EXIT_APP_FAIL;
	}
	DataSrv->SavePRM("Export:Game/Main.prm", Desc);
	FilesToPack.Append("Export:Game/Main.prm");

	if (!Browser.IsCurrDirEmpty()) do
	{
		if (Browser.IsCurrEntryFile())
		{
			if (!Browser.GetCurrEntryName().CheckExtension("hrd")) continue;

			nString FileNoExt = Browser.GetCurrEntryName();
			FileNoExt.StripExtension();
			n_msg(VR_INFO, "Parsing level '%s'...\n", FileNoExt.CStr());

			Data::PParams LevelDesc = DataSrv->LoadHRD("Src:Game/Levels/" + Browser.GetCurrEntryName(), false);
			if (!LevelDesc.IsValid())
			{
				n_msg(VR_ERROR, "Error loading level '%s' desc...\n", FileNoExt.CStr());
				continue;
			}

			nString FileFullName = "Export:Game/Levels/" + FileNoExt + ".prm";
			DataSrv->SavePRM(FileFullName, LevelDesc);
			FilesToPack.Append(FileFullName);

			FileFullName = "Src:Game/Levels/" + FileNoExt + ".lua";
			if (IOSrv->FileExists(FileFullName))
			{
				//???or collect to batch-convert later?
				nString OutFullName = "Export:Game/Levels/" + FileNoExt + ".lua";
				//LPCSTR Args[4] = { "-in", FileFullName.CStr(), "-out", OutFullName.CStr() };
				char CmdLine[4096];
				sprintf_s(CmdLine, "-v 0 -in %s -out %s",
					IOSrv->ManglePath(FileFullName).CStr(),
					IOSrv->ManglePath(OutFullName).CStr());
				int ExitCode = RunExternalToolAsProcess(CStrID("CFLua"), CmdLine);
				FilesToPack.Append(FileFullName);
			}

			FileFullName = "Export:Game/Levels/" + FileNoExt + ".nm";
			if (IOSrv->FileExists(FileFullName)) FilesToPack.Append(FileFullName);

			// Export entities
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
