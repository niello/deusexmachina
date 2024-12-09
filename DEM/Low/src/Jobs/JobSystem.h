#pragma once
#include "Worker.h"
#include <thread>
#include <vector>
#include <string_view>
#include <map>

// A dispatcher that accepts tasks to be processed by worker threads

namespace DEM::Jobs
{

// Limited by the worker index size and the number of bits in a _WaitJobWorkerMask / _WaitCounterWorkerMask
constexpr size_t MAX_WORKERS = std::min<size_t>(std::numeric_limits<uint8_t>().max(), sizeof(size_t) * 8);

struct CWorkerConfig
{
	std::string_view ThreadNamePrefix;
	uint8_t          ThreadCount;
	uint8_t          JobTypeMask; //???!!!or array / vector/ initializer list? to enforce priority between types!

	static CWorkerConfig Normal(uint8_t Count) { return CWorkerConfig{ "Worker", Count, ENUM_MASK(EJobType::Normal) }; }
	static CWorkerConfig Sleepy(uint8_t Count) { return CWorkerConfig{ "SleepyWorker", Count, ENUM_MASK(EJobType::Sleepy) }; }
	static CWorkerConfig Default(uint8_t ReservedLimit = 0) { return Normal(static_cast<uint8_t>(std::min(std::thread::hardware_concurrency(), MAX_WORKERS - ReservedLimit))); }
};

class CJobSystem final
{
protected:

	std::unique_ptr<CWorker[]>         _Workers; // Workers aren't movable because of std::atomic in a queue
	std::vector<std::thread>           _Threads;
	std::map<std::thread::id, uint8_t> _ThreadToIndex;
	std::atomic<size_t>                _WaitJobWorkerMask = 0;        // For workers ready to do some jobs when they arrive
	std::atomic<size_t>                _WaitCounterWorkerMask = 0;    // For workers that wait for an event and don't do other jobs in the meantime, see CWorker::WaitIdle
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

	CJobSystem(std::initializer_list<CWorkerConfig> Config = { CWorkerConfig::Default() });
	~CJobSystem();

	// Private interface for workers

	bool     StartWaiting(CJobCounter Counter, CJob* pJob, EJobType JobType);
	bool     StartWaiting(CJobCounter Counter, uint8_t WorkerIndex);
	void     EndWaiting(const CJobCounter& Counter, CWorker& Worker);
	void     WakeUpWorker(uint8_t AvailableJobsMask = 0);
	void     SetWorkerWaitingJob(uint8_t Index) { _WaitJobWorkerMask.fetch_or((1 << Index), std::memory_order_seq_cst); } // See a call in CWorker::MainLoop for comments
	void     SetWorkerNotWaitingJob(uint8_t Index) { _WaitJobWorkerMask.fetch_and(~(1 << Index), std::memory_order_relaxed); }
	void     SetWorkerWaitingCounter(uint8_t Index) { _WaitCounterWorkerMask.fetch_or((1 << Index), std::memory_order_seq_cst); } // See a call in CWorker::WaitIdle for comments
	void     SetWorkerNotWaitingCounter(uint8_t Index) { _WaitCounterWorkerMask.fetch_and(~(1 << Index), std::memory_order_relaxed); }
	bool     IsWorkerSleeping(uint8_t Index) const { return (_WaitJobWorkerMask.load(std::memory_order_relaxed) & (1 << Index)) || (_WaitCounterWorkerMask.load(std::memory_order_relaxed) & (1 << Index)); }

	// Public interface

	CWorker& GetWorker(uint8_t Index) const { return _Workers[Index]; }
	CWorker* FindCurrentThreadWorker() const;
	uint8_t  FindCurrentThreadWorkerIndex() const;
	size_t   GetWorkerThreadCount() const { return _Threads.size(); }
	bool     HasJobs(uint8_t TypeMask = ~0) const;
	bool     IsTerminationRequested(bool SeqCstRead = false) const { return _TerminationRequested.load(SeqCstRead ? std::memory_order_seq_cst : std::memory_order_relaxed); }
};

}
