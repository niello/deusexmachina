#include <ContentForgeTool.h>
#include <Render/ShaderMetaCommon.h>
#include <Utils.h>
#include <ParamsUtils.h>

namespace fs = std::filesystem;

// Set working directory to $(TargetDir)
// Example args:
// -s src/materials --path Data ../../../content

class CMaterialTool : public CContentForgeTool
{
public:

	CMaterialTool(const std::string& Name, const std::string& Desc, CVersion Version) :
		CContentForgeTool(Name, Desc, Version)
	{
		// Set default before parsing command line
		_RootDir = "../../../content";
	}

	virtual bool SupportsMultithreading() const override
	{
		// FIXME: must handle duplicate targets (for example 2 metafiles writing the same resulting file, like depth_atest_ps)
		// FIXME: CStrID
		return false;
	}

	virtual bool ProcessTask(CContentForgeTask& Task) override
	{
		// TODO: check whether the metafile can be processed by this tool

		const std::string Output = ParamsUtils::GetParam<std::string>(Task.Params, "Output", std::string{});
		const std::string TaskID(Task.TaskID.CStr());
		auto DestPath = fs::path(Output) / (TaskID + ".mtl");
		if (!_RootDir.empty() && DestPath.is_relative())
			DestPath = fs::path(_RootDir) / DestPath;

		// Read effect hrd

		Data::CParams Desc;
		if (!ParamsUtils::LoadParamsFromHRD(Task.SrcFilePath.string().c_str(), Desc))
		{
			if (_LogVerbosity >= EVerbosity::Errors)
				Task.Log.LogError(Task.SrcFilePath.generic_string() + " HRD loading or parsing error");
			return false;
		}

		const std::string EffectID = ParamsUtils::GetParam<std::string>(Desc, "Effect", std::string{});
		if (EffectID.empty())
		{
			Task.Log.LogError("Material must reference an effect");
			return false;
		}

		CMaterialParams MaterialParams;

		const Data::CData* pParams;
		if (ParamsUtils::TryGetParam(pParams, Desc, "Params"))
		{
			if (!pParams->IsA<Data::CParams>())
			{
				Task.Log.LogError("'Params' must be a section");
				return false;
			}

			// Get material table from the effect file

			auto Path = ResolvePathAliases(EffectID).generic_string();
			Task.Log.LogDebug("Opening effect " + Path);
			if (!GetEffectMaterialParams(MaterialParams, Path, Task.Log)) return false;
		}

		// Write resulting file

		fs::create_directories(DestPath.parent_path());

		std::ofstream File(DestPath, std::ios_base::binary | std::ios_base::trunc);

		return SaveMaterial(File, EffectID, MaterialParams, pParams ? pParams->GetValue<Data::CParams>() : Data::CParams(), Task.Log);
	}
};

int main(int argc, const char** argv)
{
	CMaterialTool Tool("cf-material", "DeusExMachina material compiler", { 1, 0, 0 });
	return Tool.Execute(argc, argv);
}
