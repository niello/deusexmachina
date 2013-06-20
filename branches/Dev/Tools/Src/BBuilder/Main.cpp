#include "Main.h"

#include <IO/IOServer.h>
#include <Data/DataServer.h>
#include <IO/FSBrowser.h>
#include "ncmdlineargs.h"
#include <conio.h>

	// Find game file. If use Src, export it here, else ?update from Src or just notify the Src is changed?
	// Iterate over all level files. If use Src, export it here, else ?update from Src or just notify the Src is changed?
	//   Export resources referenced by level entities
	// Iterate over all entity templates
	//   Export resources referenced by template entities

	// ConvertResource(Config)
	// ConvertResource(ID, Convertor, Format / AddInfo / CommandLine)

Ptr<IO::CIOServer>		IOServer;
Ptr<Data::CDataServer>	DataServer;

int main(int argc, const char** argv)
{
	nCmdLineArgs args(argc, argv);

	// If true, application will wait for key before exit
	bool WaitKey = args.GetBoolArg("-waitkey");

	// Verbosity level, where 0 is silence
	Verbose = args.GetIntArg("-v");

	// Project directory, where all content is placed. Will be a base directory for all data.
	nString ProjDir = args.GetStringArg("-proj");
	ProjDir.ConvertBackslashes();
	ProjDir.StripTrailingSlash();

	if (ProjDir.IsEmpty()) EXIT_APP_FAIL;

	n_msg(VR_ALWAYS, SEP_LINE"BBuilder v"VERSION" for DeusExMachina engine\n(c) Vladimir \"Niello\" Orlov 2011-2013\n"SEP_LINE"\n");

	IOServer = n_new(IO::CIOServer);
	IOSrv->SetAssign("Proj", ProjDir);

	n_msg(VR_INFO, "Project directory: %s\n\n", ProjDir.CStr());

	DataServer = n_new(Data::CDataServer);

	// Parse levels

	nString PathLevels = "Proj:Src/Game/Levels";
	IO::CFSBrowser Browser;
	if (!Browser.SetAbsolutePath(PathLevels))
	{
		n_msg(VR_ERROR, "Could not open directory 'Proj:Src/Game/Levels' for reading!\n", PathLevels.CStr());
		EXIT_APP_FAIL;
	}

	if (!Browser.IsCurrDirEmpty()) do
	{
		if (Browser.IsCurrEntryFile())
		{
			if (!Browser.GetCurrEntryName().CheckExtension("hrd")) continue;

			nString FileNoExt = Browser.GetCurrEntryName();
			FileNoExt.StripExtension();
			n_msg(VR_INFO, "Parsing level '%s'...\n", FileNoExt.CStr());

			// Call level parsing function
			//   Read HRD
			//   Export HRD as PRM or by scheme that allows diff
			//   Export script, if found
			//   Export navmesh, if found
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

	Release();
	return NoError ? 0 : 1;
}
//---------------------------------------------------------------------

//???need?
void Release()
{
	DataServer = NULL;
	IOServer = NULL;
}
//---------------------------------------------------------------------
