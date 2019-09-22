#include "ContentForgeTool.h"
#include <ParamsUtils.h>
#include <Utils.h>
#include <CLI11.hpp>
#include <algorithm>

namespace fs = std::filesystem;

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
		fs::path FullSrcPath = fs::path(SrcPath).is_absolute() ? SrcPath : _RootDir + SrcPath;

		auto Status = fs::status(FullSrcPath);

		// Skip inexistent source even if corresponding metafile exists
		if (!fs::exists(Status))
		{
			if (_LogVerbosity >= EVerbosity::Warning)
				std::cout << FullSrcPath << " not found, skipped" << std::endl;
			continue;
		}

		if (fs::is_directory(Status))
		{
			// Parse the directory recursively for metafiles
			// All metafiles found here definitely exist
			//std::files
			for (const auto& Entry : fs::recursive_directory_iterator(FullSrcPath))
			{
				if (!Entry.is_directory() && Entry.path().extension() == ".meta")
					ProcessMetafile(Entry.path());
			}
		}
		else
		{
			if (FullSrcPath.extension() != ".meta")
			{
				// This is not a metafile, try to find and process its corresponding metafile
				FullSrcPath += ".meta";

				if (!fs::exists(FullSrcPath))
				{
					if (_LogVerbosity >= EVerbosity::Warning)
						std::cout << FullSrcPath << " not found, skipped" << std::endl;
					continue;
				}
			}

			ProcessMetafile(FullSrcPath);
		}
	}

	return 0;
}
//---------------------------------------------------------------------

void CContentForgeTool::ProcessMetafile(const std::filesystem::path& Path)
{
	//!!!detect duplicates! don't process one metafile twice!

	//!!!DBG TMP!
	std::cout << Path << std::endl;
}
//---------------------------------------------------------------------
