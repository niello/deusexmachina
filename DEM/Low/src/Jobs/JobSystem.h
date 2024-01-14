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
	uint8_t          ThreadCount;
	uint8_t          JobTypeMask; //???!!!or array / vector/ initializer list? to enforce priority between types!

	static CWorkerConfig Normal(uint8_t Count) { return CWorkerConfig{ "Worker", Count, ENUM_MASK(EJobType::Normal) }; }
	static CWorkerConfig Sleepy(uint8_t Count) { return CWorkerConfig{ "SleepyWorker", Count, ENUM_MASK(EJobType::Sleepy) }; }
	static CWorkerConfig Default() { return Normal(std::thread::hardware_concurrency()); }
};

class CJobSystem final
{
protected:

	std::unique_ptr<CWorker[]>         _Workers; // Workers aren't movable because of std::atomic in a queue
	std::vector<std::thread>           _Threads;
	std::map<std::thread::id, uint8_t> _ThreadToIndex;
	std::atomic<size_t>                _SleepingWorkerMask = 0;
	std::atomic<bool>                  _TerminationRequested = false;

	// An unified waiting record for jobs added with AddWaitingJob and worker threads waiting on a counter
	struct CWaiter
	{
		CJob*        pJob; // nullptr - WorkerIndex, else JobType
		union
		{
			uint8_t  WorkerIndex;
			EJobType JobType;
		};

		CWaiter(CJob* pJob_, EJobType JobType_) : pJob(pJob_), JobType(JobType_) {}
		CWaiter(uint8_t WorkerIndex_) : pJob(nullptr), WorkerIndex(WorkerIndex_) {}
	};

	std::mutex                                    _WaitListMutex;
	std::unordered_multimap<CJobCounter, CWaiter> _WaitList; //???!!!Need a list of CWaiter* nodes? Use lock-free hash map?!

	// TODO: add pool or handle manager for dependency counters, not to allocate a new shared_ptr with atomic each time the counter is needed

public:

	// Limited by the worker index size and the number of bits in a _SleepingWorkerMask
	inline static constexpr size_t MAX_WORKERS = std::min<size_t>(std::numeric_limits<uint8_t>().max(), sizeof(size_t) * 8);

	CJobSystem(std::initializer_list<CWorkerConfig> Config = { CWorkerConfig::Default() });
	~CJobSystem();

	// Private interface for workers

	bool     StartWaiting(CJobCounter Counter, CJob* pJob, EJobType JobType);
	bool     StartWaiting(CJobCounter Counter, uint8_t WorkerIndex);
	void     EndWaiting(const CJobCounter& Counter, CWorker& Worker);
	void     WakeUpWorker(uint8_t AvailableJobsMask = 0);
	void     SetWorkerSleeping(uint8_t Index) { _SleepingWorkerMask.fetch_or((1 << Index), std::memory_order_seq_cst); } // See a call in CWorker::MainLoop for comments
	void     SetWorkerAwakened(uint8_t Index) { _SleepingWorkerMask.fetch_and(~(1 << Index), std::memory_order_relaxed); }
	bool     IsWorkerSleeping(uint8_t Index) const { return _SleepingWorkerMask.load(std::memory_order_relaxed) & (1 << Index); }

	// Public interface

	CWorker& GetWorker(uint8_t Index) const { return _Workers[Index]; }
	CWorker* FindCurrentThreadWorker() const;
	uint8_t  FindCurrentThreadWorkerIndex() const;
	size_t   GetWorkerThreadCount() const { return _Threads.size(); }
	bool     HasJobs(uint8_t TypeMask = ~0) const;
	bool     IsTerminationRequested(bool SeqCstRead = false) const { return _TerminationRequested.load(SeqCstRead ? std::memory_order_seq_cst : std::memory_order_relaxed); }
};

}
