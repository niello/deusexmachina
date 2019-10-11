#pragma once
#include <sstream>

enum EVerbosity
{
	Always = 0,
	Errors,
	Warnings,
	Info,
	Debug
};

inline const char* GetSeverityPrefix(EVerbosity Level)
{
	switch (Level)
	{
		case EVerbosity::Errors: return "E: ";
		case EVerbosity::Warnings: return "W: ";
		case EVerbosity::Info: return "I: ";
		case EVerbosity::Debug: return "D: ";
		default: return "";
	}
}
//---------------------------------------------------------------------

//???add simple thread-unsafe std::cout logger?

class CThreadSafeLog
{
private:

	EVerbosity _Verbosity;
	std::string _Prefix;
	std::ostringstream _Stream; // Cache logs in a separate stream for thread safety
	char _LineEnd = '\n';

public:

	CThreadSafeLog(const std::string& Prefix, EVerbosity Verbosity)
		: _Prefix(Prefix)
		, _Verbosity(Verbosity)
		, _LineEnd(_Stream.widen('\n'))
	{
	}

	std::ostringstream& GetStream() { return _Stream; }
	char GetLineEnd() const { return _LineEnd; }

	void LogAlways(const std::string& Message) { Log(EVerbosity::Always, Message.c_str()); }
	void LogError(const std::string& Message) { Log(EVerbosity::Errors, Message.c_str()); }
	void LogWarning(const std::string& Message) { Log(EVerbosity::Warnings, Message.c_str()); }
	void LogInfo(const std::string& Message) { Log(EVerbosity::Info, Message.c_str()); }
	void LogDebug(const std::string& Message) { Log(EVerbosity::Debug, Message.c_str()); }

	void Log(EVerbosity Level, const char* pMessage)
	{
		if (_Verbosity >= Level)
			_Stream << GetSeverityPrefix(Level) << _Prefix << pMessage << _LineEnd;
	}
};
