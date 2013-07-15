#include <ConsoleApp.h>
#include <Data/Params.h>

// Main header

typedef CDict<CStrID, CArray<CString>> CToolFileLists;

#define TOOL_NAME	"BBuilder"
#define VERSION		"3.0 alpha"

#define EXIT_APP_FAIL	return ExitApp(false, WaitKey)
#define EXIT_APP_OK		return ExitApp(true, WaitKey)

extern bool					ExportDescs;
extern bool					ExportResources;
extern bool					ExportShaders;
extern int					Verbose;
extern int					ExternalVerbosity;
extern CArray<CString>		FilesToPack;
extern CToolFileLists		InFileLists;
extern CToolFileLists		OutFileLists;

bool	ProcessLevel(const Data::CParams& LevelDesc, const CString& Name);
bool	ProcessFrameShader(const Data::CParams& Desc);
bool	ProcessDesc(const CString& SrcFilePath, const CString& ExportFilePath);
bool	ProcessResourceDesc(const CString& RsrcFileName, const CString& ExportFileName);
bool	ProcessQuestsInFolder(const CString& SrcPath, const CString& ExportPath);
void	BatchToolInOut(CStrID Name, const CString& InStr, const CString& OutStr);
int		RunExternalToolAsProcess(CStrID Name, LPSTR pCmdLine, LPCSTR pWorkingDir = NULL);
int		RunExternalToolBatch(CStrID Tool, int Verb, LPCSTR pExtraCmdLine = NULL, LPCSTR pWorkingDir = NULL);
bool	PackFiles(const CArray<CString>& FilesToPack, const CString& PkgFileName, const CString& PkgRoot, CString PkgRootDir);
int		ExitApp(bool NoError, bool WaitKey);

inline bool IsFileAdded(const CString& File)
{
	return FilesToPack.FindIndexSorted(File) != INVALID_INDEX;
}
//---------------------------------------------------------------------
