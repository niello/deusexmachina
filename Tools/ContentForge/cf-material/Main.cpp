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
	}

	virtual bool SupportsMultithreading() const override
	{
		// FIXME: must handle duplicate targets (for example 2 metafiles writing the same resulting file, like depth_atest_ps)
		// FIXME: CStrID
		return false;
	}

	virtual ETaskResult ProcessTask(CContentForgeTask& Task) override
	{
		const auto DestPath = GetOutputPath(Task.Params) / (Task.TaskID.ToString() + ".mtl");

		// Read effect hrd

		Data::CParams Desc;
		if (!ParamsUtils::LoadParamsFromHRD(Task.SrcFilePath.string().c_str(), Desc))
		{
			if (_LogVerbosity >= EVerbosity::Errors)
				Task.Log.LogError(Task.SrcFilePath.generic_string() + " HRD loading or parsing error");
			return ETaskResult::Failure;
		}

		const std::string EffectID = ParamsUtils::GetParam<std::string>(Desc, "Effect", std::string{});
		if (EffectID.empty())
		{
			Task.Log.LogError("Material must reference an effect");
			return ETaskResult::Failure;
		}

		CMaterialParams MaterialParams;

		const Data::CData* pParams;
		if (ParamsUtils::TryGetParam(pParams, Desc, "Params"))
		{
			if (!pParams->IsA<Data::CParams>())
			{
				Task.Log.LogError("'Params' must be a section");
				return ETaskResult::Failure;
			}

			// Get material table from the effect file

			auto Path = ResolvePathAliases(EffectID, _PathAliases).generic_string();
			Task.Log.LogDebug("Opening effect " + Path);
			if (!GetEffectMaterialParams(MaterialParams, Path, Task.Log)) return ETaskResult::Failure;
		}

		// Write resulting file

		fs::create_directories(DestPath.parent_path());

		std::ofstream File(DestPath, std::ios_base::binary | std::ios_base::trunc);

		const bool Result = SaveMaterial(File, EffectID, MaterialParams, pParams ? pParams->GetValue<Data::CParams>() : Data::CParams(), Task.Log);
		return Result ? ETaskResult::Success : ETaskResult::Failure;
	}
};

int main(int argc, const char** argv)
{
	CMaterialTool Tool("cf-material", "DeusExMachina material compiler", { 1, 0, 0 });
	return Tool.Execute(argc, argv);
}
