#include <IO/IOServer.h>
#include <IO/PathUtils.h>
#include <Data/Params.h>
#include <Data/Buffer.h>
#include <Data/HRDParser.h>
#include <Data/StringTokenizer.h>
#include <ConsoleApp.h>

//!!!tiyxml2 mustn't be linked!

#define TOOL_NAME	"CFLua"
#define VERSION		"1.0"

int		Verbose = VL_ERROR;

int		ExitApp(int Code, bool WaitKey);

bool	LuaCompile(char* pData, uint Size, LPCSTR Name, LPCSTR pFileOut);
bool	LuaCompileClass(Data::CParams& LoadedHRD, LPCSTR Name, LPCSTR pFileOut);
void	LuaRelease();

bool ProcessSingleFile(const CString& InFileName, const CString& OutFileName, bool CheckFileType = true, bool IsClass = false)
{
	CString Name = PathUtils::ExtractFileNameWithoutExtension(InFileName);

	Data::CBuffer Buffer;
	if (!IOSrv->LoadFileToBuffer(InFileName, Buffer))
	{
		FAIL;
	}

	Data::PParams ClassDesc;
	if (CheckFileType || IsClass)
	{
		Data::CHRDParser Parser;
		IsClass = Parser.ParseBuffer((LPCSTR)Buffer.GetPtr(), Buffer.GetSize(), ClassDesc);
		if (!CheckFileType && !IsClass)
		{
			n_msg(VL_ERROR, "Class HDR parsing failed\n", InFileName.CStr());
			FAIL;
		}
	}

	IOSrv->CreateDirectory(PathUtils::ExtractDirName(OutFileName));

	if (IsClass) return LuaCompileClass(*ClassDesc, Name.CStr(), OutFileName.CStr());
	else return LuaCompile((char*)Buffer.GetPtr(), Buffer.GetSize(), Name.CStr(), OutFileName.CStr());
}
//---------------------------------------------------------------------

int main(int argc, const char** argv)
{
	nCmdLineArgs Args(argc, argv);

	bool WaitKey = Args.GetBoolArg("-waitkey");
	Verbose = Args.GetIntArg("-v");
	const char* pIn = Args.GetStringArg("-in");
	const char* pOut = Args.GetStringArg("-out");

	if (!pIn || !pOut || !*pIn || !*pOut) return ExitApp(ERR_INVALID_CMD_LINE, WaitKey);

	IO::CIOServer IOServer;

	CArray<CString> InList, OutList;

	{
		Data::CStringTokenizer StrTok(pIn);
		while (StrTok.GetNextToken(';'))
			InList.Add(CString(StrTok.GetCurrToken()));
	}

	{
		Data::CStringTokenizer StrTok(pOut);
		while (StrTok.GetNextToken(';'))
			OutList.Add(CString(StrTok.GetCurrToken()));
	}

	if (InList.GetCount() != OutList.GetCount()) return ExitApp(ERR_INVALID_CMD_LINE, WaitKey);

	for (int i = 0; i < InList.GetCount(); ++i)
	{
		const CString& InRec = InList[i];
		const CString& OutRec = OutList[i];

		n_msg(VL_INFO, "Compiling pair %d: '%s' -> '%s'\n", i, InRec.CStr(), OutRec.CStr());

		bool Dir = IOSrv->DirectoryExists(InRec);
		if (!Dir && IOSrv->DirectoryExists(OutRec)) return ExitApp(ERR_IN_OUT_TYPES_DONT_MATCH, WaitKey);

		if (Dir)
		{
			//PathExport + Name + ".cls"
			return ExitApp(ERR_NOT_IMPLEMENTED_YET, WaitKey);
		}
		else
		{
			if (!IOSrv->FileExists(InRec)) return ExitApp(ERR_IN_NOT_FOUND, WaitKey);
			if (!ProcessSingleFile(InRec, OutRec)) return ExitApp(ERR_MAIN_FAILED, WaitKey);
		}
	}

	return ExitApp(SUCCESS, WaitKey);
}
//---------------------------------------------------------------------

int ExitApp(int Code, bool WaitKey)
{
	if (Code != SUCCESS) n_msg(VL_ERROR, TOOL_NAME" v"VERSION": Error occured with code %d\n", Code);

	LuaRelease();

	if (WaitKey)
	{
		Sys::Log("\nPress any key to exit...\n");
		_getch();
	}

	return Code;
}
//---------------------------------------------------------------------
