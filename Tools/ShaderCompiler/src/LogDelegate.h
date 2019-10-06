#pragma once
#include <Logging.h>

namespace DEMShaderCompiler
{

class ILogDelegate
{
public:

	void LogAlways(const char* pMessage) { Log(EVerbosity::Always, pMessage); }
	void LogError(const char* pMessage) { Log(EVerbosity::Errors, pMessage); }
	void LogWarning(const char* pMessage) { Log(EVerbosity::Warnings, pMessage); }
	void LogInfo(const char* pMessage) { Log(EVerbosity::Info, pMessage); }
	void LogDebug(const char* pMessage) { Log(EVerbosity::Debug, pMessage); }

	virtual void Log(EVerbosity Level, const char* pMessage) = 0;
};

}
