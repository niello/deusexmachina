#include <ConsoleApp.h>
#include <Data/Dictionary.h>
#include <Data/HashTable.h>
#include <Data/Params.h>
#include <Data/DataArray.h>
#include <Data/FourCC.h>

// Main header

typedef CDict<CStrID, CArray<CString>> CToolFileLists;
typedef CHashTable<CString, Data::CFourCC> CPropCodeMap;

#define TOOL_NAME	"BBuilder"
#define VERSION		"3.0.1"

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
extern CPropCodeMap			PropCodes;

void	ConvertPropNamesToFourCC(Data::PDataArray Props);
bool	ProcessLevel(const Data::CParams& LevelDesc, const CString& Name);
bool	ProcessFrameShader(const Data::CParams& Desc);
bool	ProcessDesc(const char* pSrcFilePath, const char* pExportFilePath);
bool	ProcessUISettingsDesc(const char* pSrcFilePath, const char* pExportFilePath);
bool	ProcessResourceDesc(const CString& RsrcFileName, const CString& ExportFileName);
bool	ProcessDescsInFolder(const CString& SrcPath, const CString& ExportPath);
bool	ProcessEntityTplsInFolder(const CString& SrcPath, const CString& ExportPath);
bool	ProcessQuestsInFolder(const CString& SrcPath, const CString& ExportPath);
bool	ProcessSOActionTplsDesc(const CString& SrcFilePath, const CString& ExportFilePath);
void	BatchToolInOut(CStrID Name, const CString& InStr, const CString& OutStr);
int		RunExternalToolAsProcess(CStrID Name, char* pCmdLine, const char* pWorkingDir = NULL);
int		RunExternalToolBatch(CStrID Tool, int Verb, const char* pExtraCmdLine = NULL, const char* pWorkingDir = NULL);
bool	PackFiles(const CArray<CString>& FilesToPack, const CString& PkgFileName, const CString& PkgRoot, CString PkgRootDir);
int		ExitApp(bool NoError, bool WaitKey);

inline bool IsFileAdded(const CString& File)
{
	return FilesToPack.FindIndexSorted(File) != INVALID_INDEX;
}
//---------------------------------------------------------------------
