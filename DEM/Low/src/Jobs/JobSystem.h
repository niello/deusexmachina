#pragma once
#include "Worker.h"
#include <thread>
#include <vector>
#include <string_view>
#include <map>

// A dispatcher that accepts tasks to be processed by worker threads

namespace DEM::Jobs
{

class CJobSystem final
{
protected:

	std::vector<CWorker>                _Workers;
	std::vector<std::thread>            _Threads;
	std::map<std::thread::id, uint32_t> _ThreadToIndex;

public:

	CJobSystem(uint32_t ThreadCount = std::thread::hardware_concurrency(), std::string_view ThreadNamePrefix = "Worker");
	~CJobSystem();
};

}
