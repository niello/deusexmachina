#include "ContentForgeTool.h"
#include <ParamsUtils.h>
#include <Utils.h>
#include <CLI11.hpp>
#include <algorithm>

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
	CLIApp.add_option("-s,--src", _SrcPathes, "Source file or metafile");
	CLIApp.add_option("-v", _LogVerbosity, "Verbosity level")->check(CLI::Range(0, 5));
	CLIApp.add_option("-w,--waitkey", _WaitKey, "Wait for key press after the tool has finished");
}
//---------------------------------------------------------------------

int CContentForgeTool::Execute(int argc, const char** argv)
{
	// Process command line and print some info about the tool running

	{
		CLI::App CLIApp(_Desc, _Name);
		ProcessCommandLine(CLIApp);
		CLI11_PARSE(CLIApp, argc, argv);
	}

	if (!_RootDir.empty() && _RootDir.back() != '/' && _RootDir.back() != '\\')
		_RootDir.push_back('/');

	if (_LogVerbosity >= EVerbosity::Info)
		std::cout << _Name << " v" << static_cast<uint32_t>(_Version.Major) <<
			'.' << static_cast<uint32_t>(_Version.Minor) <<
			'.' << static_cast<uint32_t>(_Version.Patch);

	// Build a list of conversion tasks from source metafiles

	//!!!each meta file can have multiple tasks = multiple target resources!
	// multithreaded src verify + task prepare
	// then multithreaded processing
	for (const auto& SrcPath : _SrcPathes)
	{
		std::string FullSrcPath = IsPathAbsolute(SrcPath) ? SrcPath : _RootDir + SrcPath;

		if (DirectoryExists(FullSrcPath.c_str()))
		{
			// Parse the directory recursively for metafiles
			// All metafiles found here definitely exist
		}
		else
		{
			if (ExtractExtension(FullSrcPath) != "meta")
			{
				// This is not a metafile, try to find and process its corresponding metafile
				FullSrcPath += ".meta";
			}

			if (!FileExists(FullSrcPath.c_str()))
			{
				if (_LogVerbosity >= EVerbosity::Warning)
					std::cout << FullSrcPath << " not found, skipped" << std::endl;
				continue;
			}

			// process source metafile
		}
	}

	return 0;
}
//---------------------------------------------------------------------
