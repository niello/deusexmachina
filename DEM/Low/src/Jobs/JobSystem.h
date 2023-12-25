#pragma once
#include "Worker.h"
#include <vector>
#include <string_view>

// A dispatcher that accepts tasks to be processed by worker threads

namespace DEM::Jobs
{

class CJobSystem
{
protected:

	std::vector<CWorker> _Workers;

public:

	CJobSystem(uint32_t ThreadCount = std::thread::hardware_concurrency(), std::string_view ThreadNamePrefix = "Worker");
};

}
