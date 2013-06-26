#include <ConsoleApp.h>
#include <Data/Params.h>

// Main header

#define TOOL_NAME	"BBuilder"
#define VERSION		"3.0 alpha"

#define EXIT_APP_FAIL	return ExitApp(false, WaitKey)
#define EXIT_APP_OK		return ExitApp(true, WaitKey)

extern bool					ExportDescs;
extern int					Verbose;
extern nArray<nString>		FilesToPack;
extern nArray<nString>		CFLuaIn;
extern nArray<nString>		CFLuaOut;

bool	ProcessLevel(const Data::CParams& LevelDesc, const nString& Name);
int		RunExternalToolBatch(CStrID Tool, int Verb, const nArray<nString>& InList, const nArray<nString>& OutList, LPCSTR pWorkingDir = NULL);
bool	PackFiles(const nArray<nString>& FilesToPack, const nString& PkgFileName, const nString& PkgRoot, nString PkgRootDir);
int		ExitApp(bool NoError, bool WaitKey);

inline bool IsFileAdded(const nString& File)
{
	return FilesToPack.BinarySearchIndex(File) != INVALID_INDEX;
}
//---------------------------------------------------------------------
