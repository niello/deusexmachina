#include "ContentForgeTool.h"
#include <ParamsUtils.h>
#include <Utils.h>
#include <CLI11.hpp>
#include <algorithm>
#include <thread>
#include <conio.h>

namespace fs = std::filesystem;

CContentForgeTool::CContentForgeTool(const std::string& Name, const std::string& Desc, CVersion Version)
	: _Name(Name)
	, _Desc(Desc)
	, _Version(Version)
	, _RootDir("../../../content") // Set default for typical layout before parsing the command line
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
	CLIApp.add_option("--root", _RootDir, "Root folder for all project resources");
	CLIApp.add_option("--out", _OutDir, "Output folder, default is root");
	CLIApp.add_option("-s,--src", _SrcPathes, "Source asset file or metafile or folder with them");
	CLIApp.add_option("-v", _LogVerbosity, "Verbosity level")->check(
		CLI::Range(static_cast<int>(EVerbosity::Always), static_cast<int>(EVerbosity::Debug)));
	CLIApp.add_option("-w,--waitkey", _WaitKey, "Wait for key press after the tool has finished");

	// Path aliases (assigns)
	CLIApp.add_option("--path", [this](CLI::results_t vals)
	{
		const size_t PairCount = vals.size() / 2;
		for (size_t i = 0; i < PairCount; ++i)
			_PathAliases[vals[i * 2]] = vals[i * 2 + 1];
		return true;
	}, "Path alias for resource location resolving")->type_name("KEY VALUE")->type_size(-2);
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

	if (!_OutDir.empty() && _OutDir.back() != '/' && _OutDir.back() != '\\')
		_OutDir.push_back('/');

	// Run custom initialization code

	{
		const int Code = Init();
		if (Code != 0) return Code;
	}

	// Print tool name and version

	const auto LineEnd = std::cout.widen('\n');

	if (_LogVerbosity >= EVerbosity::Info)
	{
		std::cout << _Name << " v" << static_cast<uint32_t>(_Version.Major) <<
			'.' << static_cast<uint32_t>(_Version.Minor) <<
			'.' << static_cast<uint32_t>(_Version.Patch) <<
			" (CWD: " << std::filesystem::current_path().generic_string() << ')' << LineEnd;

		//const auto Now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
		//std::cout << "Started: " << std::ctime(&Now) << LineEnd;
	}

	// Build a list of conversion tasks from source metafiles

	{
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
					std::cout << FullSrcPath.generic_string() << " not found, skipped" << LineEnd;
				continue;
			}

			if (fs::is_directory(Status))
			{
				// Parse the directory recursively for metafiles
				for (const auto& Entry : fs::recursive_directory_iterator(FullSrcPath))
				{
					if (!Entry.is_directory() && Entry.path().extension() == ".meta")
						ProcessMetafile(Entry.path(), Processed);
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
							std::cout << FullSrcPath.generic_string() << " not found, skipped" << LineEnd;
						continue;
					}
				}

				ProcessMetafile(FullSrcPath, Processed);
			}
		}
	}

	if (_LogVerbosity >= EVerbosity::Debug)
		std::cout << LineEnd;

	// Run tasks in parallel

	//???pass task count to SupportsMultithreading for perf tuning?
	if (_Tasks.size() > 1 && SupportsMultithreading())
	{
		//!!!DBG TMP!
		for (auto& Task : _Tasks)
		{
			if (_LogVerbosity >= EVerbosity::Info)
			{
				Task.Log.GetStream() << "Source: " << Task.SrcFilePath.generic_string() << Task.Log.GetLineEnd();
				Task.Log.GetStream() << "Task: " << Task.TaskID.CStr() << Task.Log.GetLineEnd();
				Task.Log.GetStream() << "Thread: " << std::this_thread::get_id() << Task.Log.GetLineEnd();
			}

			Task.Result = ProcessTask(Task);
		}

		typedef std::function<void(CContentForgeTask& Task)> FJob;
		/*
		const auto WorkerThreadCount = std::max(1u, std::thread::hardware_concurrency());

		for (uint32_t i = 0; i < WorkerThreadCount; ++i)
		{
			std::thread Worker([]
			{
				FJob Job;

				while (true)
				{
					//if (_Tasks.pop_front(job)) // try to grab a job from the jobPool queue
					//{
					//	// It found a job, execute it:
					//	job(); // execute job
					//	finishedLabel.fetch_add(1); // update worker label state
					//}
					//else
					//{
					//	// no job, put thread to sleep
					//	std::unique_lock<std::mutex> lock(wakeMutex);
					//	wakeCondition.wait(lock);
					//}
				}
			});

			Worker.detach(); // forget about this thread, let it do it's job in the infinite loop that we created above
		}
		*/

		// TODO: wait all threads to finish
	}
	else
	{
		for (auto& Task : _Tasks)
		{
			if (_LogVerbosity >= EVerbosity::Info)
			{
				Task.Log.GetStream() << "Source: " << Task.SrcFilePath.generic_string() << Task.Log.GetLineEnd();
				Task.Log.GetStream() << "Task: " << Task.TaskID.CStr() << Task.Log.GetLineEnd();
				Task.Log.GetStream() << "Thread: " << std::this_thread::get_id() << Task.Log.GetLineEnd();
			}

			Task.Result = ProcessTask(Task);
		}
	}

	// Flush cached logs to stdout, calculate statistics

	size_t TasksByResult[ETaskResult::COUNT] = {};

	for (auto& Task : _Tasks)
	{
		++TasksByResult[Task.Result];

		const auto LoggedString = Task.Log.GetStream().str();
		if (!LoggedString.empty())
			std::cout << LoggedString;

		if (_LogVerbosity >= EVerbosity::Info)
		{
			std::cout << "Status: ";
			switch (Task.Result)
			{
				case ETaskResult::NotStarted: std::cout << "not started"; break;
				case ETaskResult::Success: std::cout << "OK"; break;
				case ETaskResult::Failure: std::cout << "FAILED"; break;
				case ETaskResult::UpToDate: std::cout << "skipped as up to date"; break;
				default: std::cout << "unknown"; break;
			}
			std::cout << LineEnd << LineEnd;
		}
			
	}

	std::cout << _Name << ": successful: " << TasksByResult[ETaskResult::Success] <<
		", failed: " << TasksByResult[ETaskResult::Failure] <<
		", up to date: " << TasksByResult[ETaskResult::UpToDate] <<
		LineEnd << LineEnd;

	// Run custom termination code

	{
		const int Code = Term();
		if (Code != 0) return Code;
	}

	if (_WaitKey) _getch();

	return 0;
}
//---------------------------------------------------------------------

void CContentForgeTool::ProcessMetafile(const std::filesystem::path& Path, std::unordered_set<std::string>& Processed)
{
	const auto LineEnd = std::cout.widen('\n');

	// Detect already processed metafiles
	{
		std::string PathStr = fs::absolute(Path).generic_string();
		if (Processed.find(PathStr) != Processed.cend()) return;

		Processed.insert(std::move(PathStr));
	}

	if (_LogVerbosity >= EVerbosity::Debug)
		std::cout << "Processing metafile: " << Path.generic_string() << LineEnd;

	// Read metafile
	Data::CParams Meta;
	if (!ParamsUtils::LoadParamsFromHRD(Path.string().c_str(), Meta))
	{
		if (_LogVerbosity >= EVerbosity::Errors)
			std::cout << Path.generic_string() << " HRD loading or parsing error" << LineEnd;
		return;
	}

	// NB: probably some other fields may be added to the metafile, but now it must consist of sections only
	for (const auto& Param : Meta)
	{
		if (!Param.second.IsA<Data::CParams>())
		{
			if (_LogVerbosity >= EVerbosity::Errors)
				std::cout << Path.generic_string() << " must contain only per-resource sections" << LineEnd;
			return;
		}
	}

	// Read source file once, data will be shared among all tasks

	auto SrcFilePath = Path;
	SrcFilePath.replace_extension(); // Remove ".meta"

	// Add tasks for this source

	for (auto& Param : Meta)
	{
		auto& TaskParams = Param.second.GetValue<Data::CParams>();

		// Check whether the task can be processed by this tool
		// TODO: split by comma!
		const std::string ToolList = ParamsUtils::GetParam(TaskParams, "Tools", std::string{});
		if (!ToolList.empty() && ToolList != _Name) continue;

		CContentForgeTask Task(static_cast<EVerbosity>(_LogVerbosity));
		Task.SrcFilePath = SrcFilePath;
		Task.TaskID = Param.first;
		Task.Params = std::move(TaskParams);
		_Tasks.push_back(std::move(Task));
	}
}
//---------------------------------------------------------------------

std::filesystem::path CContentForgeTool::GetOutputPath(const Data::CParams& TaskParams, const char* pPathID) const
{
	std::filesystem::path Result;

	std::string PathValue;
	if (pPathID && ParamsUtils::TryGetParam(PathValue, TaskParams, pPathID))
		Result = PathValue;
	else if (ParamsUtils::TryGetParam(PathValue, TaskParams, "Output"))
		Result = PathValue;
	else
		return Result;

	if (Result.is_relative())
	{
		if (!_OutDir.empty())
			Result = _OutDir / Result;
		else if (!_RootDir.empty())
			Result = _RootDir / Result;
	}

	return Result;
}
//---------------------------------------------------------------------
