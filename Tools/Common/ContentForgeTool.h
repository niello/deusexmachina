#pragma once
#include <string>

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

class CContentForgeTool
{
protected:

	std::string _Name;
	std::string _Desc;
	CVersion _Version;

	std::string _RootDir;

	int _LogVerbosity = 2;

public:

	CContentForgeTool(const std::string& Name, const std::string& Desc, CVersion Version);

	virtual void ProcessCommandLine(CLI::App& CLIApp);

	int Execute(int argc, const char** argv);

	const std::string& GetName() const { return _Name; }
	const std::string& GetDesc() const { return _Desc; }
	const CVersion& GetVersion() const { return _Version; }
};
