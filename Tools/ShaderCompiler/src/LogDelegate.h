#pragma once

namespace DEMShaderCompiler
{

class ILogDelegate
{
public:

	// Exactly the same as in CF tools
	enum ESeverity
	{
		Always = 0,
		Errors,
		Warnings,
		Info,
		Debug
	};

	void LogAlways(const char* pMessage) { Log(ESeverity::Always, pMessage); }
	void LogError(const char* pMessage) { Log(ESeverity::Errors, pMessage); }
	void LogWarning(const char* pMessage) { Log(ESeverity::Warnings, pMessage); }
	void LogInfo(const char* pMessage) { Log(ESeverity::Info, pMessage); }
	void LogDebug(const char* pMessage) { Log(ESeverity::Debug, pMessage); }

	virtual void Log(ESeverity Level, const char* pMessage) = 0;
};

}
