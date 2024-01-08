#include "JobSystem.h"
#include <System/System.h>
#include <StdDEM.h>
#include <string>
#include <mutex>
#include <condition_variable>

namespace DEM::Jobs
{

CJobSystem::CJobSystem(std::initializer_list<CWorkerConfig> Config)
{
	ZoneScoped;

	// Allocate all workers at once. The last worker is for the current (main) thread, this simplifies a design.
	uint32_t TotalThreadCount = 0;
	for (const auto& ConfigRecord : Config)
		TotalThreadCount += ConfigRecord.ThreadCount;
	_Workers.reset(new CWorker[TotalThreadCount + 1]);

	n_assert(TotalThreadCount <= MAX_WORKERS);

	uint8_t ThreadIndex = 0;
	for (const auto& ConfigRecord : Config)
		for (uint8_t i = 0; i < ConfigRecord.ThreadCount; ++i, ++ThreadIndex)
			_Workers[ThreadIndex].Init(*this, ConfigRecord.ThreadNamePrefix.data() + std::to_string(i), ThreadIndex, ConfigRecord.JobTypeMask);

	// Init main thread with all job types allowed.
	// NB: it is not necessarily a 'main' thread in a common meaning, it is instead any thread in which the job system was created.
	_Workers[ThreadIndex].Init(*this, "MainThread", ThreadIndex);

	// NB: it is crucial not to reserve but to resize the vector because we use its
	// size as a thread count and access it in workers before we create all threads
	_Threads.resize(TotalThreadCount);

	_ThreadToIndex.emplace(std::this_thread::get_id(), TotalThreadCount);

	uint8_t ThreadsStarted = 0;
	std::mutex ThreadsStartedMutex;
	std::condition_variable ThreadsStartedCV;

	for (uint8_t i = 0; i < TotalThreadCount; ++i)
	{
		_Threads[i] = std::thread([this, i, TotalThreadCount, &ThreadsStarted, &ThreadsStartedMutex, &ThreadsStartedCV]
		{
			auto& ThisWorker = _Workers[i];

			Sys::SetCurrentThreadName(ThisWorker.GetName());
			tracy::SetThreadName(ThisWorker.GetName().c_str());

			// NB: sleepy jobs (IO, network etc) don't need an affinity because they are blocked most of the time and we want them to wake anywhere possible
			// TODO PERF: test under the real workload. Profiling shows that this may perform better or worse depending on the test itself.
			if (ThisWorker.GetJobTypeMask() & ENUM_MASK(EJobType::Normal))
				Sys::SetCurrentThreadAffinity(i);

			//???!!!TODO: raise thread priority for sleepy jobs to fight priority inversion, when some normal job waits dynamically spawned IO job?!

			// Let the system know that we are ready to go. Wake up the constructor thread if we were the last.
			{
				std::lock_guard Lock(ThreadsStartedMutex);
				_ThreadToIndex.emplace(std::this_thread::get_id(), i);
				if (++ThreadsStarted == TotalThreadCount) ThreadsStartedCV.notify_one();
			}

			ThisWorker.MainLoop();
		});
	}

	// Wait for all worker threads to start
	{
		std::unique_lock Lock(ThreadsStartedMutex);
		ThreadsStartedCV.wait(Lock, [&ThreadsStarted, TotalThreadCount]() { return ThreadsStarted == TotalThreadCount; });
	}
}
//---------------------------------------------------------------------

// NB: cancels not started jobs. To execute them wait on their completion counter(s) before destroying a system.
CJobSystem::~CJobSystem()
{
	// Set a termination flag for workers to see the request
	_TerminationRequested.store(true, std::memory_order_seq_cst);

	// Wake up all sleeping workers so that they can terminate properly
	const size_t ThreadCount = _Threads.size();
	for (size_t i = 0; i < ThreadCount; ++i)
		_Workers[i].WakeUp();

	n_assert_dbg(!_SleepingWorkerMask.load(std::memory_order_seq_cst));

	// Wait for all worker threads to terminate
	for (auto& Thread: _Threads)
		if (Thread.joinable()) Thread.join();
}
//---------------------------------------------------------------------

bool CJobSystem::StartWaiting(CJobCounter Counter, CJob* pJob, EJobType JobType)
{
	// There are no unsatisfied dependencies, return false to let the worker enqueue the job immediately
	if (!pJob || !Counter || Counter->load(std::memory_order_relaxed) == 0) return false;

	{
		std::unique_lock Lock(_WaitListMutex);

		// Counter might have changed after the previous check but before we locked the mutex, check again.
		// This check will not be reordered before the lock because locking has acquire semantics.
		// Return false to let the worker enqueue the job immediately. EndWaiting() has either finished or was
		// not called yet or is waiting for the _WaitListMutex. In any case it will not find pJob in the wait list.
		if (Counter->load(std::memory_order_relaxed) == 0) return false;

		// Only workers can call StartWaiting() and they never push the same job twice, no duplicate check is needed
		_WaitList.emplace(std::move(Counter), CWaiter(pJob, JobType));
	}

	// The job is successfully added to the wait list and will be found by the next EndWaiting()
	return true;
}
//---------------------------------------------------------------------

bool CJobSystem::StartWaiting(CJobCounter Counter, uint8_t WorkerIndex)
{
	// There are no unsatisfied dependencies, return false to let the worker continue immediately
	if (!Counter || Counter->load(std::memory_order_relaxed) == 0) return false;

	{
		std::unique_lock Lock(_WaitListMutex);

		// Counter might have changed after the previous check but before we locked the mutex, check again.
		// This check will not be reordered before the lock because locking has acquire semantics.
		// Return false to let the worker continue immediately.
		if (Counter->load(std::memory_order_relaxed) == 0) return false;

		// Don't check for duplicates, they are most likely rare and harmless
		_WaitList.emplace(std::move(Counter), CWaiter(WorkerIndex));
	}

	// The worker is successfully added to the wait list and will be awakened by the next EndWaiting()
	return true;
}
//---------------------------------------------------------------------

void CJobSystem::EndWaiting(CJobCounter Counter, CWorker& Worker)
{
	// Assume the counter being 0, otherwise this method wouldn't be called.
	// Incrementing and starting waiting on the same counter is illegal until it is removed from the wait list.

	size_t WorkersToWakeUp = 0;
	size_t NewJobCount[EJobType::Count] = {};
	{
		std::unique_lock Lock(_WaitListMutex);
		auto [ItBegin, ItEnd] = _WaitList.equal_range(Counter);
		if (ItBegin == ItEnd) return;

		// TODO: pushing jobs should not be protected by mutex but it is probably better than copying job pointers
		// to an intermediate array. If will rewrite to linked list of waiting jobs, can copy list head.
		for (auto It = ItBegin; It != ItEnd; ++It)
		{
			auto& Waiter = It->second;
			if (Waiter.pJob)
			{
				// Schedule waiting job
				Worker.Push(Waiter.JobType, Waiter.pJob);
				++NewJobCount[Waiter.JobType];
			}
			else
			{
				// Register the waiting worker for waking up
				WorkersToWakeUp |= (1 << Waiter.WorkerIndex);
			}
		}

		_WaitList.erase(ItBegin, ItEnd);
	}

	CWorker* pBegin = &_Workers[0];
	const CWorker* pEnd = pBegin + _Threads.size();

	//???TODO PERF: do need seq cst here? or is it enough to store with it?
	auto SleepingWorkerMask = _SleepingWorkerMask.load(std::memory_order_seq_cst);
	if (!SleepingWorkerMask) return;

	// Resume waiting workers first, see CWorker::WaitActive/WaitIdle(CJobCounter Counter)
	// NB: main thread included because it can be put to wait
	WorkersToWakeUp &= SleepingWorkerMask;
	for (auto pCurr = pBegin; WorkersToWakeUp && (pCurr <= pEnd); ++pCurr, WorkersToWakeUp >>= 1)
		if (WorkersToWakeUp & 1) pCurr->WakeUp();

	// Don't try to wake up already awakened workers
	SleepingWorkerMask &= ~WorkersToWakeUp;
	if (!SleepingWorkerMask) return;

	// Wake up additional workers for stealing new jobs
	// NB: main thread excluded
	//!!!FIXME PERF: can check caps matching once per thread group, caps inside a group are the same!
	for (uint8_t JobType = 0; JobType < EJobType::Count; ++JobType)
	{
		auto JobCount = NewJobCount[JobType];
		const auto JobTypeMask = ENUM_MASK(JobType);
		uint8_t WorkerBitMask = 1;
		for (auto pCurr = pBegin; JobCount && (pCurr != pEnd); ++pCurr, WorkerBitMask <<= 1)
		{
			if ((SleepingWorkerMask & WorkerBitMask) && (pCurr->GetJobTypeMask() & JobTypeMask) && pCurr->WakeUp())
			{
				// Don't try to wake up already awakened workers
				SleepingWorkerMask &= ~WorkerBitMask;
				if (!SleepingWorkerMask) return;
				--JobCount;
			}
		}
	}
}
//---------------------------------------------------------------------

void CJobSystem::WakeUpWorker(uint8_t AvailableJobsMask)
{
	//ZoneScoped;

	//???TODO PERF: do need seq cst here? or is it enough to store with it?
	auto SleepingWorkerMask = _SleepingWorkerMask.load(std::memory_order_seq_cst);
	if (!SleepingWorkerMask) return;

	CWorker* pBegin = &_Workers[0];
	const CWorker* pEnd = pBegin + _Threads.size();

	if (!AvailableJobsMask)
	{
		// TODO PERF: early exit if all types are set? don't check queues for which the bit is already set?
		// NB: main thread included because it can contain jobs for stealing by other workers
		for (auto pCurr = pBegin; pCurr <= pEnd; ++pCurr)
			AvailableJobsMask |= pCurr->CollectAvailableJobsMask();

		if (!AvailableJobsMask) return;
	}

	// Wake up the first suitable worker
	// NB: main thread excluded
	for (auto pCurr = pBegin; pCurr != pEnd; ++pCurr, SleepingWorkerMask >>= 1)
		if ((SleepingWorkerMask & 1) && (pCurr->GetJobTypeMask() & AvailableJobsMask) && pCurr->WakeUp())
			return;
}
//---------------------------------------------------------------------

bool CJobSystem::HasJobs(uint8_t TypeMask) const
{
	const auto ThreadCount = _Threads.size();
	for (uint8_t i = 0; i <= ThreadCount; ++i)
		if (_Workers[i].HasJobs(TypeMask)) return true;
	return false;
}
//---------------------------------------------------------------------

CWorker* CJobSystem::FindCurrentThreadWorker() const
{
	auto It = _ThreadToIndex.find(std::this_thread::get_id());
	return (It != _ThreadToIndex.cend()) ? &_Workers[It->second] : nullptr;
}
//---------------------------------------------------------------------

uint8_t CJobSystem::FindCurrentThreadWorkerIndex() const
{
	auto It = _ThreadToIndex.find(std::this_thread::get_id());
	return (It != _ThreadToIndex.cend()) ? It->second : std::numeric_limits<decltype(It->second)>().max();
}
//---------------------------------------------------------------------

}
