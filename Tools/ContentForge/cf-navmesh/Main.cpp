#include <ContentForgeTool.h>
#include <Utils.h>
//#include <ParamsUtils.h>
//#include <CLI11.hpp>

namespace fs = std::filesystem;

// Set working directory to $(TargetDir)
// Example args:
// -s src/FIXME

class CNavmeshTool : public CContentForgeTool
{
protected:

public:

	CNavmeshTool(const std::string& Name, const std::string& Desc, CVersion Version)
		: CContentForgeTool(Name, Desc, Version)
	{
	}

	virtual bool SupportsMultithreading() const override
	{
		// FIXME: must handle duplicate targets (for example 2 metafiles writing the same resulting file, like depth_atest_ps)
		// FIXME: CStrID
		return false;
	}

	virtual int Init() override
	{
		return 0;
	}

	virtual void ProcessCommandLine(CLI::App& CLIApp) override
	{
		CContentForgeTool::ProcessCommandLine(CLIApp);
	}

	virtual bool ProcessTask(CContentForgeTask& Task) override
	{
		// TODO: check whether the metafile can be processed by this tool

		const std::string TaskName = GetValidResourceName(Task.TaskID.ToString());

		return true;
	}
};

int main(int argc, const char** argv)
{
	CNavmeshTool Tool("cf-navmesh", "Navigation mesh generator for static level geometry", { 0, 1, 0 });
	return Tool.Execute(argc, argv);
}
