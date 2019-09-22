#pragma once
#include <Data.h>
#include <filesystem>

// Base class for different console tools

namespace CLI
{
	class App;
}

enum EVerbosity
{
	Always = 0,
	Error,
	Warning,
	Info,
	Debug
};

struct CVersion
{
	uint8_t Major = 0;
	uint8_t Minor = 0;
	uint8_t Patch = 0;
};

struct CContentForgeTask
{
	std::string SrcFilePath;
	Data::CParams Params;
};

class CContentForgeTool
{
protected:

	std::string _Name;
	std::string _Desc;
	CVersion _Version;

	std::string _RootDir;
	std::vector<std::string> _SrcPathes;
	std::vector<CContentForgeTask> _Tasks;

	int _WorkerThreadCount = 4;
	int _LogVerbosity = 2;
	bool _WaitKey = false;

	void ProcessMetafile(const std::filesystem::path& Path);

public:

	CContentForgeTool(const std::string& Name, const std::string& Desc, CVersion Version);

	virtual void ProcessCommandLine(CLI::App& CLIApp);

	int Execute(int argc, const char** argv);

	const std::string& GetName() const { return _Name; }
	const std::string& GetDesc() const { return _Desc; }
	const CVersion& GetVersion() const { return _Version; }
};
