#include <ConsoleApp.h>
#include <Data/Dictionary.h>
#include <Data/HashTable.h>
#include <Data/Params.h>
#include <Data/DataArray.h>
#include <Data/FourCC.h>

// Main header

#define TOOL_NAME	"BBuilder"
#define VERSION		"3.1.0"

#define EXIT_APP_FAIL	return ExitApp(false, WaitKey)
#define EXIT_APP_OK		return ExitApp(true, WaitKey)

typedef CDict<CStrID, CArray<CString>> CToolFileLists;
typedef CHashTable<CString, Data::CFourCC> CClassCodeMap;

extern CString			ProjectDir;
extern bool				ExportDescs;
extern bool				ExportResources;
extern bool				IncludeSM30ShadersAndEffects;
extern int				Verbose;
extern int				ExternalVerbosity;
extern CArray<CString>	FilesToPack;
extern CArray<U32>		ShadersToPack;
extern CToolFileLists	InFileLists;
extern CToolFileLists	OutFileLists;
extern CClassCodeMap	ClassToFOURCC;

void	ConvertPropNamesToFourCC(Data::PDataArray Props);
bool	ProcessLevel(const Data::CParams& LevelDesc, const CString& Name);
bool	ExportEffect(const CString& SrcFilePath, const CString& ExportFilePath, bool LegacySM30);
bool	ProcessShaderResourceDesc(const Data::CParams& Desc, bool Debug, bool OnlyMetadata, U32& OutShaderID);
bool	ProcessDesc(const char* pSrcFilePath, const char* pExportFilePath);
bool	ProcessRenderPathDesc(const char* pSrcFilePath, const char* pExportFilePath);
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

inline void AddShaderToPack(U32 ID)
{
	if (!ShadersToPack.Contains(ID)) ShadersToPack.Add(ID);
}
//---------------------------------------------------------------------
