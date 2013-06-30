#include <ConsoleApp.h>
#include <Data/Params.h>

// Main header

typedef nDictionary<CStrID, nArray<nString>> CToolFileLists;

#define TOOL_NAME	"BBuilder"
#define VERSION		"3.0 alpha"

#define EXIT_APP_FAIL	return ExitApp(false, WaitKey)
#define EXIT_APP_OK		return ExitApp(true, WaitKey)

extern bool					ExportDescs;
extern bool					ExportResources;
extern bool					ExportShaders;
extern int					Verbose;
extern int					ExternalVerbosity;
extern nArray<nString>		FilesToPack;
extern CToolFileLists		InFileLists;
extern CToolFileLists		OutFileLists;

bool	ProcessLevel(const Data::CParams& LevelDesc, const nString& Name);
bool	ProcessFrameShader(const Data::CParams& Desc);
bool	ProcessDesc(const nString& SrcFilePath, const nString& ExportFilePath);
bool	ProcessResourceDesc(const nString& RsrcFileName, const nString& ExportFileName);
bool	ProcessQuestsInFolder(const nString& SrcPath, const nString& ExportPath);
void	BatchToolInOut(CStrID Name, const nString& InStr, const nString& OutStr);
int		RunExternalToolAsProcess(CStrID Name, LPSTR pCmdLine, LPCSTR pWorkingDir = NULL);
int		RunExternalToolBatch(CStrID Tool, int Verb, LPCSTR pExtraCmdLine = NULL, LPCSTR pWorkingDir = NULL);
bool	PackFiles(const nArray<nString>& FilesToPack, const nString& PkgFileName, const nString& PkgRoot, nString PkgRootDir);
int		ExitApp(bool NoError, bool WaitKey);

inline bool IsFileAdded(const nString& File)
{
	return FilesToPack.BinarySearchIndex(File) != INVALID_INDEX;
}
//---------------------------------------------------------------------
