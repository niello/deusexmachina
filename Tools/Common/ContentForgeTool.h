#pragma once
#include <Data.h>
#include <Logging.h>
#include <filesystem>
#include <unordered_set>
#include <deque>

// Base class for different console tools

namespace CLI
{
	class App;
}

struct CVersion
{
	uint8_t Major = 0;
	uint8_t Minor = 0;
	uint8_t Patch = 0;
};

enum ETaskResult
{
	NotStarted = 0,
	Success,
	Failure,
	UpToDate,

	COUNT
};

struct CContentForgeTask
{
	std::filesystem::path SrcFilePath;
	CStrID TaskID;
	Data::CParams Params;
	CThreadSafeLog Log;
	ETaskResult Result = ETaskResult::NotStarted;

	CContentForgeTask(EVerbosity LogVerbosity) : Log("", LogVerbosity) {}
};

class CContentForgeTool
{
protected:

	std::string _Name;
	std::string _Desc;
	CVersion _Version;

	std::string _RootDir;
	std::map<std::string, std::filesystem::path> _PathAliases;
	std::vector<std::string> _SrcPathes;
	std::deque<CContentForgeTask> _Tasks;

	int _LogVerbosity;
	bool _WaitKey = false;

	void ProcessMetafile(const std::filesystem::path& Path, std::unordered_set<std::string>& Processed);

public:

	CContentForgeTool(const std::string& Name, const std::string& Desc, CVersion Version);

	virtual int Init() { return 0; }
	virtual int Term() { return 0; }
	virtual bool SupportsMultithreading() const { return false; }
	virtual void ProcessCommandLine(CLI::App& CLIApp);
	virtual void ProcessTask(CContentForgeTask& Task) = 0;

	int Execute(int argc, const char** argv);

	std::filesystem::path GetPath(const Data::CParams& TaskParams, const char* pPathID) const;
	std::filesystem::path ResolvePathAliases(const std::string& Path) const;

	const std::string& GetName() const { return _Name; }
	const std::string& GetDesc() const { return _Desc; }
	const CVersion& GetVersion() const { return _Version; }
};
