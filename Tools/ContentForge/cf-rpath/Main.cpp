#include <ContentForgeTool.h>
#include <Utils.h>
#include <HRDParser.h>
#include <thread>
#include <mutex>
#include <iostream>

namespace fs = std::filesystem;

// Set working directory to $(TargetDir)
// Example args:
// -s src/rpaths --path Data ../../../content

class CRenderPathTool : public CContentForgeTool
{
private:

	std::mutex COutMutex;

public:

	CRenderPathTool(const std::string& Name, const std::string& Desc, CVersion Version) :
		CContentForgeTool(Name, Desc, Version)
	{
		// Set default before parsing command line
		_RootDir = "../../../content";
	}

	virtual bool SupportsMultithreading() const override
	{
		// FIXME: CStrID
		return false;
	}

	virtual bool ProcessTask(CContentForgeTask& Task) override
	{
		// TODO: check whether the metafile can be processed by this tool

		const std::string Output = GetParam<std::string>(Task.Params, "Output", std::string{});
		const std::string TaskID(Task.TaskID.CStr());
		auto DestPath = fs::path(Output) / (TaskID + ".eff");
		if (!_RootDir.empty() && DestPath.is_relative())
			DestPath = fs::path(_RootDir) / DestPath;

		CThreadSafeLog Log("", static_cast<EVerbosity>(_LogVerbosity));

		// FIXME: can move to the common code
		if (_LogVerbosity >= EVerbosity::Info)
		{
			Log.GetStream() << "Source: " << Task.SrcFilePath.generic_string() << Log.GetLineEnd();
			Log.GetStream() << "Task: " << Task.TaskID.CStr() << Log.GetLineEnd();
			Log.GetStream() << "Thread: " << std::this_thread::get_id() << Log.GetLineEnd();
		}

		// Read render path hrd

		Data::CParams Desc;
		{
			std::vector<char> In;
			if (!ReadAllFile(Task.SrcFilePath.string().c_str(), In, false))
			{
				Log.LogError(Task.SrcFilePath.generic_string() + " reading error");
				return false;
			}

			Data::CHRDParser Parser;
			if (!Parser.ParseBuffer(In.data(), In.size(), Desc))
			{
				Log.LogError(Task.SrcFilePath.generic_string() + " HRD parsing error");
				return false;
			}
		}

		// Build and verify global parameter table

		std::map<uint32_t, std::string> SerializedGlobals;
		auto ItEffects = Desc.find(CStrID("Effects"));
		if (ItEffects != Desc.cend())
		{
			if (!ItEffects->second.IsA<Data::CDataArray>())
			{
				Log.LogError("'Phases' must be an array of pathes to .eff files");
				return false;
			}

			const auto& EffectPathes = ItEffects->second.GetValue<Data::CDataArray>();
			for (const auto& EffectPathData : EffectPathes)
			{
				if (!EffectPathData.IsA<std::string>())
				{
					Log.LogError("Wrong data in 'Effects' array, all elements must be strings");
					return false;
				}

				auto Path = ResolvePathAliases(EffectPathData.GetValue<std::string>());
				Log.LogDebug("Opening effect " + Path.generic_string());

				std::ifstream File(Path, std::ios_base::binary);
				if (!File)
				{
					Log.LogError("Can't open effect " + Path.generic_string());
					return false;
				}

				// For each shader format merge and verify global metadata
			}
		}

		// Process phases

		auto ItPhases = Desc.find(CStrID("Phases"));
		if (ItPhases == Desc.cend() || !ItPhases->second.IsA<Data::CParams>())
		{
			Log.LogError("'Phases' must be a section");
			return false;
		}

		const auto& PhaseDescs = ItPhases->second.GetValue<Data::CParams>();
		for (const auto& PhasePair : PhaseDescs)
		{
			if (!PhasePair.second.IsA<Data::CParams>())
			{
				Log.LogError("Phase '" + PhasePair.first.ToString() + "' must be a section");
				return false;
			}

			const auto& PhaseDesc = PhasePair.second.GetValue<Data::CParams>();
			//
		}

		//???warn no render targets/DS-buffers referenced by index?
		//???use names instead of indices?

		// Write resulting file

		fs::create_directories(DestPath.parent_path());

		std::ofstream File(DestPath, std::ios_base::binary);
		if (!File)
		{
			Log.LogError("Error opening an output file");
			return false;
		}

		WriteStream<uint32_t>(File, 'RPTH');                   // Format magic value
		WriteStream<uint32_t>(File, 0x00010000);               // Version 0.1.0.0
		WriteStream<uint32_t>(File, SerializedGlobals.size()); // Shader format count

		// Finish task

		const auto LoggedString = Log.GetStream().str();
		if (!LoggedString.empty())
		{
			// Flush cached logs to stdout
			// TODO: move log flushing to the end of CContentForgeTool::Execute? No locking needed at all there.
			std::lock_guard<std::mutex> Lock(COutMutex);
			std::cout << LoggedString;
		}

		// FIXME: must be thread-safe, also can move to the common code
		//if (_LogVerbosity >= EVerbosity::Debug)
		//	std::cout << "Status: " << (Ok ? "OK" : "FAIL") << LineEnd << LineEnd;

		return true;
	}
};

int main(int argc, const char** argv)
{
	CRenderPathTool Tool("cf-rpath", "DeusExMachina render path compiler", { 0, 1, 0 });
	return Tool.Execute(argc, argv);
}
