#pragma once
#include <string>

// Executes a subprocess synchronously and returns its exit code

class CThreadSafeLog;

int RunSubprocess(const std::string& CommandLine, CThreadSafeLog* pLog = nullptr);
