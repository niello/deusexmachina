#include "ContentForgeTool.h"
#include <CLI11.hpp>

CContentForgeTool::CContentForgeTool(const std::string& Name, const std::string& Desc, CVersion Version)
	: _Name(Name)
	, _Desc(Desc)
	, _Version(Version)
{
}
//---------------------------------------------------------------------

void CContentForgeTool::ProcessCommandLine(CLI::App& CLIApp)
{
	CLIApp.add_option("-r,--root", _RootDir, "Root folder for all project resources");
	CLIApp.add_option("-v", _LogVerbosity, "Verbosity level")->check(CLI::Range(0, 5));
}
//---------------------------------------------------------------------

int CContentForgeTool::Execute(int argc, const char** argv)
{
	{
		CLI::App CLIApp(_Desc, _Name);
		ProcessCommandLine(CLIApp);
		CLI11_PARSE(CLIApp, argc, argv);
	}

	if (!_RootDir.empty() && _RootDir.back() != '/' && _RootDir.back() != '\\')
		_RootDir.push_back('/');

	std::cout << _Name << " v" << static_cast<uint32_t>(_Version.Major) <<
		'.' << static_cast<uint32_t>(_Version.Minor) <<
		'.' << static_cast<uint32_t>(_Version.Patch);

	return 0;
}
//---------------------------------------------------------------------
