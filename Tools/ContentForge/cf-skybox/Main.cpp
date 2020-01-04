#include <ContentForgeTool.h>
#include <Render/ShaderMetaCommon.h>
#include <Utils.h>
#include <ParamsUtils.h>
#include <CLI11.hpp>

namespace fs = std::filesystem;

// Set working directory to $(TargetDir)
// Example args:
// -s src/sky

class CSkyboxTool : public CContentForgeTool
{
protected:

	Data::CSchemeSet _SceneSchemes;

	std::map<std::string, std::string> _EffectsByType;
	std::map<std::string, std::string> _EffectParamAliases;

	std::string _ResourceRoot;
	std::string _SchemeFile;
	std::string _SettingsFile;
	bool        _CalcIBL = false;
	bool        _OutputBin = false;
	bool        _OutputHRD = false; // For debug purposes, saves scene hierarchies in a human-readable format

public:

	CSkyboxTool(const std::string& Name, const std::string& Desc, CVersion Version)
		: CContentForgeTool(Name, Desc, Version)
		, _ResourceRoot("Data:")
	{
		// Set default before parsing command line
		_SchemeFile = "../schemes/scene.dss";
		_SettingsFile = "../schemes/settings.hrd";
	}

	virtual bool SupportsMultithreading() const override
	{
		// FIXME: must handle duplicate targets (for example 2 metafiles writing the same resulting file, like depth_atest_ps)
		// FIXME: CStrID
		return false;
	}

	virtual int Init() override
	{
		if (_ResourceRoot.empty())
			if (_LogVerbosity >= EVerbosity::Warnings)
				std::cout << "Resource root is empty, external references may not be resolved from the game!";

		if (!_OutputHRD) _OutputBin = true;

		if (_OutputBin)
		{
			if (!ParamsUtils::LoadSchemes(_SchemeFile.c_str(), _SceneSchemes))
			{
				std::cout << "Couldn't load scene binary serialization scheme from " << _SchemeFile;
				return 2;
			}
		}

		{
			Data::CParams EffectSettings;
			if (!ParamsUtils::LoadParamsFromHRD(_SettingsFile.c_str(), EffectSettings))
			{
				std::cout << "Couldn't load effect settings from " << _SettingsFile;
				return 3;
			}

			const Data::CParams* pMap;
			if (ParamsUtils::TryGetParam(pMap, EffectSettings, "Effects"))
			{
				for (const auto& Pair : *pMap)
					_EffectsByType.emplace(Pair.first.ToString(), Pair.second.GetValue<std::string>());
			}
			if (ParamsUtils::TryGetParam(pMap, EffectSettings, "Params"))
			{
				for (const auto& Pair : *pMap)
					_EffectParamAliases.emplace(Pair.first.ToString(), Pair.second.GetValue<std::string>());
			}
		}

		return 0;
	}

	virtual void ProcessCommandLine(CLI::App& CLIApp) override
	{
		//???use --project-file instead of --res-root + --settings?
		CContentForgeTool::ProcessCommandLine(CLIApp);
		CLIApp.add_option("--res-root", _ResourceRoot, "Resource root prefix for referencing external subresources by path");
		CLIApp.add_option("--scheme,--schema", _SchemeFile, "Scene binary serialization scheme file path");
		CLIApp.add_option("--settings", _SettingsFile, "Settings file path");
		CLIApp.add_flag("-t,--txt", _OutputHRD, "Output scenes in a human-readable format, suitable for debugging only");
		CLIApp.add_flag("-b,--bin", _OutputBin, "Output scenes in a binary format, suitable for loading into the engine");
		CLIApp.add_flag("--ibl", _CalcIBL, "Calculate image-based lighting resources (IEM, PMREM)");
	}

	fs::path GetPath(const Data::CParams& TaskParams, const char* pPathID)
	{
		fs::path Result;

		std::string PathValue;
		if (ParamsUtils::TryGetParam(PathValue, TaskParams, pPathID))
			Result = PathValue;
		else if (ParamsUtils::TryGetParam(PathValue, TaskParams, "Output"))
			Result = PathValue;
		else return Result;

		if (!_RootDir.empty() && Result.is_relative())
			Result = fs::path(_RootDir) / Result;

		return Result;
	}

	const std::string& GetEffectParamID(const std::string& Alias)
	{
		auto It = _EffectParamAliases.find(Alias);
		return (It == _EffectParamAliases.cend()) ? Alias : It->second;
	}

	virtual bool ProcessTask(CContentForgeTask& Task) override
	{
		// TODO: check whether the metafile can be processed by this tool

		const std::string TaskName = Task.TaskID.ToString();

		// Copy skybox cubemap to resources

		// Write material file

		// Calculate IBL resources

		// Write scene file

		Data::CParams Result;

		Data::CDataArray Attributes;
		{
			Data::CParams Attribute;
			Attribute.emplace_back(CStrID("Class"), 'SKBA'); //std::string("Frame::CSkyboxAttribute"));
			//Attribute.emplace_back(CStrID("CDLODFile"), CDLODID);
			Attributes.push_back(std::move(Attribute));
		}
		{
			Data::CParams Attribute;
			Attribute.emplace_back(CStrID("Class"), 'NAAL'); //std::string("Frame::CAmbientLightAttribute"));
			//Attribute.emplace_back(CStrID("CDLODFile"), CDLODID);
			Attributes.push_back(std::move(Attribute));
		}
		Result.emplace_back(CStrID("Attrs"), std::move(Attributes));

		const fs::path OutPath = GetPath(Task.Params, "Output");

		if (_OutputHRD)
		{
			const auto DestPath = OutPath / (TaskName + ".hrd");
			if (!ParamsUtils::SaveParamsToHRD(DestPath.string().c_str(), Result))
			{
				Task.Log.LogError("Error serializing " + TaskName + " to text");
				return false;
			}
		}

		if (_OutputBin)
		{
			const auto DestPath = OutPath / (Task.TaskID.ToString() + ".scn");
			if (!ParamsUtils::SaveParamsByScheme(DestPath.string().c_str(), Result, CStrID("SceneNode"), _SceneSchemes))
			{
				Task.Log.LogError("Error serializing " + TaskName + " to binary");
				return false;
			}
		}

		return true;
	}
};

int main(int argc, const char** argv)
{
	CSkyboxTool Tool("cf-skybox", "Cubemap to DeusExMachina skybox asset converter", { 0, 1, 0 });
	return Tool.Execute(argc, argv);
}
