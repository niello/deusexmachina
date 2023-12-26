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

	std::unique_ptr<CWorker[]>          _Workers; // Workers aren't movable because of std::atomic in a queue
	std::vector<std::thread>            _Threads;
	std::map<std::thread::id, uint32_t> _ThreadToIndex;
	std::atomic<bool>                   _TerminationRequested = false;

	//???instead of main thread queue can create CWorker MainThreadWorker? could call its main loop for some seconds or for a number of jobs. Also can push jobs to it! Store in the same array, as a first or last one?

public:

	CJobSystem(uint32_t ThreadCount = std::thread::hardware_concurrency(), std::string_view ThreadNamePrefix = "Worker");
	~CJobSystem();

	CWorker& GetWorker(uint32_t Index) const { return _Workers[Index]; }
	CWorker& GetMainThreadWorker() const { return _Workers[_Threads.size()]; }
	size_t GetWorkerThreadCount() const { return _Threads.size(); }
	bool   IsTerminationRequested() const { return _TerminationRequested; }
};

}
