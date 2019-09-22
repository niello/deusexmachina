#include "ContentForgeTool.h"
#include <HRDParser.h>
#include <Utils.h>
#include <CLI11.hpp>
#include <algorithm>
#include <conio.h>

namespace fs = std::filesystem;

CContentForgeTool::CContentForgeTool(const std::string& Name, const std::string& Desc, CVersion Version)
	: _Name(Name)
	, _Desc(Desc)
	, _Version(Version)
#if _DEBUG
	, _LogVerbosity(static_cast<int>(EVerbosity::Debug))
#else
	, _LogVerbosity(static_cast<int>(EVerbosity::Warnings))
#endif
{
}
//---------------------------------------------------------------------

void CContentForgeTool::ProcessCommandLine(CLI::App& CLIApp)
{
	CLIApp.add_option("-r,--root", _RootDir, "Root folder for all project resources");
	CLIApp.add_option("-s,--src", _SrcPathes, "Source file or metafile");
	CLIApp.add_option("-v", _LogVerbosity, "Verbosity level")->check(
		CLI::Range(static_cast<int>(EVerbosity::Always), static_cast<int>(EVerbosity::Debug)));
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
			'.' << static_cast<uint32_t>(_Version.Patch) << std::endl;

	// Build a list of conversion tasks from source metafiles

	{
		Data::CHRDParser Parser;
		std::unordered_set<std::string> Processed;
		for (const auto& SrcPath : _SrcPathes)
		{
			// TODO: remove sections part from the path, process sections later
			// For example: path[section1,section2,sec*_debug*]
			// Select some FS-forbidden character(s) as a path/sections separator

			fs::path FullSrcPath = fs::path(SrcPath).is_absolute() ? SrcPath : _RootDir + SrcPath;
			FullSrcPath = FullSrcPath.lexically_normal();

			auto Status = fs::status(FullSrcPath);

			// Skip inexistent source even if corresponding metafile exists
			if (!fs::exists(Status))
			{
				if (_LogVerbosity >= EVerbosity::Warnings)
					std::cout << FullSrcPath.generic_string() << " not found, skipped" << std::endl;
				continue;
			}

			if (fs::is_directory(Status))
			{
				// Parse the directory recursively for metafiles
				for (const auto& Entry : fs::recursive_directory_iterator(FullSrcPath))
				{
					if (!Entry.is_directory() && Entry.path().extension() == ".meta")
						ProcessMetafile(Entry.path(), Parser, Processed);
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
						if (_LogVerbosity >= EVerbosity::Warnings)
							std::cout << FullSrcPath.generic_string() << " not found, skipped" << std::endl;
						continue;
					}
				}

				ProcessMetafile(FullSrcPath, Parser, Processed);
			}
		}
	}

	// Run tasks in parallel

	for (auto& Task : _Tasks)
	{
		auto Job = [this](CContentForgeTask& Task)
		{
			ProcessTask(Task);
		};
		Job(Task);
	}

	if (_WaitKey) _getch();

	return 0;
}
//---------------------------------------------------------------------

void CContentForgeTool::ProcessMetafile(const std::filesystem::path& Path, Data::CHRDParser& Parser, std::unordered_set<std::string>& Processed)
{
	// Detect already processed metafiles
	{
		std::string PathStr = fs::absolute(Path).generic_string();
		if (Processed.find(PathStr) != Processed.cend()) return;

		Processed.insert(std::move(PathStr));
	}

	if (_LogVerbosity >= EVerbosity::Debug)
		std::cout << "Processing metafile: " << Path.generic_string() << std::endl;

	// Read metafile
	Data::CParams Meta;
	{
		std::vector<char> In;
		if (!ReadAllFile(Path.string().c_str(), In))
		{
			if (_LogVerbosity >= EVerbosity::Errors)
				std::cout << Path.generic_string() << " reading error" << std::endl;
			return;
		}

		Parser.ParseBuffer(In.data(), In.size(), Meta);
	}

	// NB: probably some other fields may be added to the metafile, but now it must consist of sections only
	for (const auto& Param : Meta)
	{
		if (!Param.second.IsA<Data::CParams>())
		{
			if (_LogVerbosity >= EVerbosity::Errors)
				std::cout << Path.generic_string() << " must contain only per-resource sections" << std::endl;
			return;
		}
	}

	// Read source file once, data will be shared among all tasks

	auto SrcFilePath = Path;
	SrcFilePath.replace_extension(); // Remove ".meta"

	auto SrcFileData = std::make_shared<std::vector<char>>();
	if (!ReadAllFile(SrcFilePath.string().c_str(), *SrcFileData))
	{
		if (_LogVerbosity >= EVerbosity::Errors)
			std::cout << SrcFilePath.generic_string() << " reading error" << std::endl;
		return;
	}

	// Add tasks for this source

	for (auto& Param : Meta)
	{
		CContentForgeTask Task;
		Task.SrcFilePath = SrcFilePath;
		Task.SrcFileData = SrcFileData;
		Task.TaskID = Param.first;
		Task.Params = std::move(Param.second.GetValue<Data::CParams>());
		_Tasks.push_back(std::move(Task));
	}
}
//---------------------------------------------------------------------
