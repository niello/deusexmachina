#include <ContentForgeTool.h>
#include <Utils.h>
#include <HRDParser.h>

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

		const std::string Output = GetParam<std::string>(Task.Params, "Output", std::string{});
		const std::string TaskID(Task.TaskID.CStr());
		auto DestPath = fs::path(Output) / (TaskID + ".eff");
		if (!_RootDir.empty() && DestPath.is_relative())
			DestPath = fs::path(_RootDir) / DestPath;

		// Read effect hrd

		Data::CParams Desc;
		{
			std::vector<char> In;
			if (!ReadAllFile(Task.SrcFilePath.string().c_str(), In, false))
			{
				Task.Log.LogError(Task.SrcFilePath.generic_string() + " reading error");
				return false;
			}

			Data::CHRDParser Parser;
			if (!Parser.ParseBuffer(In.data(), In.size(), Desc))
			{
				Task.Log.LogError(Task.SrcFilePath.generic_string() + " HRD parsing error");
				return false;
			}
		}

		return true;
	}
};

int main(int argc, const char** argv)
{
	CMaterialTool Tool("cf-material", "DeusExMachina material compiler", { 0, 1, 0 });
	return Tool.Execute(argc, argv);
}
