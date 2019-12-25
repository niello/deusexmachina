#include <ContentForgeTool.h>
#include <Utils.h>
#include <ParamsUtils.h>

namespace fs = std::filesystem;

// Set working directory to $(TargetDir)
// Example args:
// -s src/terrain

class CL3DTTool : public CContentForgeTool
{
public:

	CL3DTTool(const std::string& Name, const std::string& Desc, CVersion Version) :
		CContentForgeTool(Name, Desc, Version)
	{
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
		auto DestPath = fs::path(Output) / (TaskID + ".cdlod");
		if (!_RootDir.empty() && DestPath.is_relative())
			DestPath = fs::path(_RootDir) / DestPath;

		// Read project XML

		// Merge alpha maps

		// Write material

		// Write scene file

		return false;
	}
};

int main(int argc, const char** argv)
{
	CL3DTTool Tool("cf-l3dt", "L3DT (Large 3D Terrain) to DeusExMachina scene asset converter", { 0, 1, 0 });
	return Tool.Execute(argc, argv);
}
