#include <ContentForgeTool.h>
#include <SceneTools.h>
#include <Render/ShaderMetaCommon.h>
#include <Utils.h>
#include <ParamsUtils.h>
#include <Subprocess.h>
#include <CLI11.hpp>

namespace fs = std::filesystem;

// Set working directory to $(TargetDir)
// Example args:
// -s src/sky --path Data ../../../content --ibl -b -t

class CSkyboxTool : public CContentForgeTool
{
protected:

	Data::CSchemeSet _SceneSchemes;
	CSceneSettings   _Settings;

	std::string _ResourceRoot;
	std::string _SchemeFile;
	std::string _SettingsFile;
	bool        _IBL = false;
	bool        _RecalcIBL = false;
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

		if (!LoadSceneSettings(_SettingsFile, _Settings)) return 3;

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
		CLIApp.add_flag("--ibl", _IBL, "Add image-based lighting resources (IEM, PMREM) to the asset");
		CLIApp.add_flag("-r", _RecalcIBL, "Recalculate image-based lighting resources (IEM, PMREM) even if they exist");
	}

	virtual ETaskResult ProcessTask(CContentForgeTask& Task) override
	{
		const std::string TaskName = GetValidResourceName(Task.TaskID.ToString());

		std::string SkyboxFileName = Task.SrcFilePath.filename().generic_string();
		ToLower(SkyboxFileName);

		// Generate material

		std::string MaterialID;

		const auto TexturePath = GetOutputPath(Task.Params, "TextureOutput");

		auto EffectIt = _Settings.EffectsByType.find("MetallicRoughnessSkybox");
		if (EffectIt == _Settings.EffectsByType.cend() || EffectIt->second.empty())
		{
			Task.Log.LogError("Material type MetallicRoughnessTerrain has no mapped DEM effect file in effect settings");
			return ETaskResult::Failure;
		}

		CMaterialParams MtlParamTable;
		auto Path = ResolvePathAliases(EffectIt->second, _PathAliases).generic_string();
		Task.Log.LogDebug("Opening effect " + Path);
		if (!GetEffectMaterialParams(MtlParamTable, Path, Task.Log))
		{
			Task.Log.LogError("Error reading material param table for effect " + Path);
			return ETaskResult::Failure;
		}

		Data::CParams MtlParams;

		const auto SkyboxTextureID = _Settings.GetEffectParamID("SkyboxTexture");
		if (MtlParamTable.HasResource(SkyboxTextureID))
		{
			auto DestPath = TexturePath / SkyboxFileName;
			fs::create_directories(DestPath.parent_path());
			std::error_code ec;
			if (!fs::copy_file(Task.SrcFilePath, DestPath, fs::copy_options::overwrite_existing, ec))
			{
				Task.Log.LogError("Error copying texture from " + Task.SrcFilePath.generic_string() + " to " + DestPath.generic_string() + ": " + ec.message());
				return ETaskResult::Failure;
			}

			MtlParams.emplace_back(CStrID(SkyboxTextureID), _ResourceRoot + fs::relative(DestPath, _RootDir).generic_string());
		}
		else
		{
			Task.Log.LogError("No SkyboxTexture (" + SkyboxTextureID + ") parameter found in a material");
			return ETaskResult::Failure;
		}

		{
			auto DestPath = GetOutputPath(Task.Params, "MaterialOutput") / (TaskName + ".mtl");

			fs::create_directories(DestPath.parent_path());

			std::ofstream File(DestPath, std::ios_base::binary | std::ios_base::trunc);

			if (!SaveMaterial(File, EffectIt->second, MtlParamTable, MtlParams, Task.Log)) return ETaskResult::Failure;

			MaterialID = _ResourceRoot + fs::relative(DestPath, _RootDir).generic_string();
		}

		// Calculate IBL resources

		std::string IEMID, PMREMID;

		if (_IBL)
		{
			// See https://seblagarde.wordpress.com/2012/06/10/amd-cubemapgen-for-physically-based-rendering/

			// Calculate IEM

			auto IEMPath = TexturePath / (fs::path(SkyboxFileName).filename().replace_extension().string() + "_iem.dds");
			IEMID = _ResourceRoot + fs::relative(IEMPath, _RootDir).generic_string();
			if (_RecalcIBL || !fs::exists(IEMPath))
			{
				const int IEMDimensionSize = ParamsUtils::GetParam(Task.Params, "IEMDimensionSize", 128);
				const std::string IEMFormat = ParamsUtils::GetParam(Task.Params, "IEMFormat", std::string("A8R8G8B8"));

				const std::string IEMCmdLine = "cubemapgen/mcmg.exe"
					" -IrradianceCubemap"
					" -importCubeDDS:\"" + Task.SrcFilePath.string() + "\""
					" -exportCubeDDS"
					" -exportFilename:\"" + IEMPath.string() + "\""
					" -exportSize:" + std::to_string(IEMDimensionSize) +
					" -exportPixelFormat:" + IEMFormat +
					" -consoleErrorOutput"
					" -exit";
				if (RunSubprocess(IEMCmdLine, &Task.Log) != 0)
				{
					Task.Log.LogError("Error generating irradiance environment map (IEM)");
					return ETaskResult::Failure;
				}
			}

			// Calculate PMREM (Mipmap mode)

			auto PMREMPath = TexturePath / (fs::path(SkyboxFileName).filename().replace_extension().string() + "_pmrem.dds");
			PMREMID = _ResourceRoot + fs::relative(PMREMPath, _RootDir).generic_string();
			if (_RecalcIBL || !fs::exists(PMREMPath))
			{
				const int PMREMDimensionSize = ParamsUtils::GetParam(Task.Params, "PMREMDimensionSize", 256);
				const std::string PMREMFormat = ParamsUtils::GetParam(Task.Params, "PMREMFormat", std::string("A8R8G8B8"));
				const int PMREMMipCount = ParamsUtils::GetParam(Task.Params, "PMREMMipCount", 7);
				const float PMREMGlossScale = ParamsUtils::GetParam(Task.Params, "PMREMGlossScale", 10.f);
				const float PMREMGlossBias = ParamsUtils::GetParam(Task.Params, "PMREMGlossBias", 1.f);

				const std::string PMREMCmdLine = "cubemapgen/mcmg.exe"
					" -filterTech:CosinePower"
					" -CosinePowerMipmapChainMode:Mipmap"
					" -NumMipmap:" + std::to_string(PMREMMipCount) +
					" -GlossScale:" + std::to_string(PMREMGlossScale) +
					" -GlossBias:" + std::to_string(PMREMGlossBias) +
					" -importCubeDDS:\"" + Task.SrcFilePath.string() + "\""
					" -LightingModel:PhongBRDF"
					" -ExcludeBase"
					" -solidAngleWeighting"
					" -edgeFixupTech:Warp"
					" -importDegamma:1.0"
					" -exportGamma:1.0 "
					" -exportCubeDDS"
					" -exportFilename:\"" + PMREMPath.string() + "\""
					" -exportSize:" + std::to_string(PMREMDimensionSize) +
					" -exportPixelFormat:" + PMREMFormat +
					" -exportMipChain"
					" -consoleErrorOutput"
					" -exit";
				if (RunSubprocess(PMREMCmdLine, &Task.Log) != 0)
				{
					Task.Log.LogError("Error generating prefiltered mipmaped radiance environment map (PMREM)");
					return ETaskResult::Failure;
				}
			}
		}

		// Write scene file

		Data::CParams Result;

		Data::CDataArray Attributes;

		{
			Data::CParams Attribute;
			Attribute.emplace_back(CStrID("Class"), 'SKBA'); // Frame::CSkyboxAttribute
			Attribute.emplace_back(CStrID("Material"), MaterialID);
			Attributes.push_back(std::move(Attribute));
		}

		if (_IBL)
		{
			Data::CParams Attribute;
			Attribute.emplace_back(CStrID("Class"), 'NAAL'); // Frame::CAmbientLightAttribute
			Attribute.emplace_back(CStrID("IrradianceMap"), IEMID);
			Attribute.emplace_back(CStrID("RadianceEnvMap"), PMREMID);
			Attributes.push_back(std::move(Attribute));
		}

		Result.emplace_back(CStrID("Attrs"), std::move(Attributes));

		const fs::path OutPath = GetOutputPath(Task.Params);

		if (_OutputHRD)
		{
			const auto DestPath = OutPath / (TaskName + ".hrd");
			if (!ParamsUtils::SaveParamsToHRD(DestPath.string().c_str(), Result))
			{
				Task.Log.LogError("Error serializing " + TaskName + " to text");
				return ETaskResult::Failure;
			}
		}

		if (_OutputBin)
		{
			const auto DestPath = OutPath / (TaskName + ".scn");
			if (!ParamsUtils::SaveParamsByScheme(DestPath.string().c_str(), Result, CStrID("SceneNode"), _SceneSchemes))
			{
				Task.Log.LogError("Error serializing " + TaskName + " to binary");
				return ETaskResult::Failure;
			}
		}

		return ETaskResult::Success;
	}
};

int main(int argc, const char** argv)
{
	CSkyboxTool Tool("cf-skybox", "Cubemap to DeusExMachina skybox asset converter", { 1, 0, 0 });
	return Tool.Execute(argc, argv);
}
