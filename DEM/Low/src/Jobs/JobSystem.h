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

	std::mutex                          _WaitJobsMutex;
	std::condition_variable             _WaitJobsCV;

	std::mutex                                                             _WaitListMutex;
	std::unordered_multimap<std::shared_ptr<std::atomic<uint32_t>>, CJob*> _WaitList; //???!!!Need a list of CJob* waiting nodes? Use lock-free hash map?!

	// TODO: add pool or handle manager for dependency counters, not to allocate a new shared_ptr with atomic each time the counter is needed

public:

	CJobSystem(bool Sleepy = false, uint32_t ThreadCount = std::thread::hardware_concurrency(), std::string_view ThreadNamePrefix = "Worker");
	~CJobSystem();

	// Private interface for workers

	bool     StartWaiting(std::shared_ptr<std::atomic<uint32_t>> Counter, CJob* pJob);
	void     EndWaiting(std::shared_ptr<std::atomic<uint32_t>> Counter, CWorker& Worker);

	void     WakeUpWorkers(size_t Count);
	void     WakeUpAllWorkers();

	template<typename F>
	DEM_FORCE_INLINE void PutCurrentWorkerToSleepUntil(F Condition)
	{
		std::unique_lock Lock(_WaitJobsMutex);
		_WaitJobsCV.wait(Lock, Condition);
	}

	// Public interface

	CWorker& GetWorker(uint32_t Index) const { return _Workers[Index]; }
	CWorker& GetMainThreadWorker() const { return _Workers[_Threads.size()]; }
	CWorker* FindCurrentThreadWorker() const;
	uint32_t FindCurrentThreadWorkerIndex() const;
	size_t   GetWorkerThreadCount() const { return _Threads.size(); }
	bool     HasJobs() const;
	bool     IsTerminationRequested() const { return _TerminationRequested; }
};

}
