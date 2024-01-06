#pragma once
#include "Worker.h"
#include <thread>
#include <vector>
#include <string_view>
#include <map>

// A dispatcher that accepts tasks to be processed by worker threads

namespace DEM::Jobs
{

struct CWorkerConfig
{
	std::string_view ThreadNamePrefix;
	uint32_t         ThreadCount;
	uint8_t          JobTypeMask; //???!!!or array / vector/ initializer list? to enforce priority between types!

	static CWorkerConfig Normal(uint32_t Count) { return CWorkerConfig{ "Worker", Count, ENUM_MASK(EJobType::Normal) }; }
	static CWorkerConfig Sleepy(uint32_t Count) { return CWorkerConfig{ "SleepyWorker", Count, ENUM_MASK(EJobType::Sleepy) }; }
	static CWorkerConfig Default() { return Normal(std::thread::hardware_concurrency()); }
};

class CJobSystem final
{
protected:

	std::unique_ptr<CWorker[]>          _Workers; // Workers aren't movable because of std::atomic in a queue
	std::vector<std::thread>            _Threads;
	std::map<std::thread::id, uint32_t> _ThreadToIndex;
	std::atomic<bool>                   _TerminationRequested = false;

	// An unified waiting record for jobs added with AddWaitingJob and worker threads waiting on a counter
	struct CWaiter
	{
		CJob*        pJob; // nullptr - WorkerIndex, else JobType
		union
		{
			uint32_t WorkerIndex;
			EJobType JobType;
		};

		CWaiter(CJob* pJob_, EJobType JobType_) : pJob(pJob_), JobType(JobType_) {}
		CWaiter(uint32_t WorkerIndex_) : pJob(nullptr), WorkerIndex(WorkerIndex_) {}
	};

	std::mutex                                    _WaitListMutex;
	std::unordered_multimap<CJobCounter, CWaiter> _WaitList; //???!!!Need a list of CWaiter* nodes? Use lock-free hash map?!

	// TODO: add pool or handle manager for dependency counters, not to allocate a new shared_ptr with atomic each time the counter is needed

public:

	CJobSystem(std::initializer_list<CWorkerConfig> Config = { CWorkerConfig::Default() });
	~CJobSystem();

	// Private interface for workers

	bool     StartWaiting(CJobCounter Counter, CJob* pJob, EJobType JobType);
	bool     StartWaiting(CJobCounter Counter, uint32_t WorkerIndex);
	void     EndWaiting(CJobCounter Counter, CWorker& Worker);
	void     WakeUpWorker(uint8_t AvailableJobsMask);

	// Public interface

	CWorker& GetWorker(uint32_t Index) const { return _Workers[Index]; }
	CWorker* FindCurrentThreadWorker() const;
	uint32_t FindCurrentThreadWorkerIndex() const;
	size_t   GetWorkerThreadCount() const { return _Threads.size(); }
	bool     HasJobs(uint8_t TypeMask = ~0) const;
	uint8_t  CollectAvailableJobsMask() const;
	bool     IsTerminationRequested(bool SeqCstRead = false) const { return _TerminationRequested.load(SeqCstRead ? std::memory_order_seq_cst : std::memory_order_relaxed); }
};

}
