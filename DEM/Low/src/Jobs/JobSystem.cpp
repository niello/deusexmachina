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
	// Allocate all workers at once. The last worker is for the current (main) thread, this simplifies a design.
	uint32_t TotalThreadCount = 0;
	for (const auto& ConfigRecord : Config)
		TotalThreadCount += ConfigRecord.ThreadCount;
	_Workers.reset(new CWorker[TotalThreadCount + 1]);

	uint32_t ThreadIndex = 0;
	for (const auto& ConfigRecord : Config)
	{
		for (uint32_t i = 0; i < ConfigRecord.ThreadCount; ++i, ++ThreadIndex)
			_Workers[ThreadIndex].Init(*this, ConfigRecord.ThreadNamePrefix.data() + std::to_string(i), ThreadIndex, ConfigRecord.JobTypeMask);
	}

	// Init main thread with all job types allowed.
	// NB: it is not necessarily a 'main' thread in a common meaning, it is instead any thread in which the job system was created.
	_Workers[ThreadIndex].Init(*this, "MainThread", ThreadIndex);

	// NB: it is crucial not to reserve but to resize the vector because we use its
	// size as a thread count and access it in workers before we create all threads
	_Threads.resize(TotalThreadCount);

	_ThreadToIndex.emplace(std::this_thread::get_id(), TotalThreadCount);

	uint32_t ThreadsStarted = 0;
	std::mutex ThreadsStartedMutex;
	std::condition_variable ThreadsStartedCV;

	for (uint32_t i = 0; i < TotalThreadCount; ++i)
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

bool CJobSystem::StartWaiting(CJobCounter Counter, uint32_t WorkerIndex)
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
				// Resume waiting worker, see CWorker::WaitActive/WaitIdle(CJobCounter Counter)
				// FIXME: worker's _WaitJobsMutex will be locked inside our _WaitListMutex lock. Looks safe but can delay WakeUp after unlock just in case.
				_Workers[Waiter.WorkerIndex].WakeUp();
			}
		}

		_WaitList.erase(ItBegin, ItEnd);
	}

	// Wake up workers for stealing new jobs
	//!!!FIXME PERF: can check caps matching once per thread group, caps inside a group are the same!
	const auto ThreadCount = _Threads.size();
	for (uint8_t JobType = 0; JobType < EJobType::Count; ++JobType)
		for (uint32_t i = 0; NewJobCount[JobType] && i < ThreadCount; ++i)
			if ((_Workers[i].GetJobTypeMask() & ENUM_MASK(JobType)) && _Workers[i].WakeUp())
				--NewJobCount[JobType];
}
//---------------------------------------------------------------------

void CJobSystem::WakeUpWorker(uint8_t AvailableJobsMask)
{
	if (!AvailableJobsMask) return;

	const auto ThreadCount = _Threads.size();
	for (uint32_t i = 0; i < ThreadCount; ++i)
		if ((_Workers[i].GetJobTypeMask() & AvailableJobsMask) && _Workers[i].WakeUp())
			return;
}
//---------------------------------------------------------------------

bool CJobSystem::HasJobs(uint8_t TypeMask) const
{
	const auto ThreadCount = _Threads.size();
	for (uint32_t i = 0; i <= ThreadCount; ++i)
		if (_Workers[i].HasJobs(TypeMask)) return true;
	return false;
}
//---------------------------------------------------------------------

uint8_t CJobSystem::CollectAvailableJobsMask() const
{
	// TODO PERF: early exit if all types are set? don't check queues for which the bit is already set?
	uint8_t Mask = 0;
	const auto ThreadCount = _Threads.size();
	for (uint32_t i = 0; i <= ThreadCount; ++i)
		Mask |= _Workers[i].CollectAvailableJobsMask();
	return Mask;
}
//---------------------------------------------------------------------

CWorker* CJobSystem::FindCurrentThreadWorker() const
{
	auto It = _ThreadToIndex.find(std::this_thread::get_id());
	return (It != _ThreadToIndex.cend()) ? &_Workers[It->second] : nullptr;
}
//---------------------------------------------------------------------

uint32_t CJobSystem::FindCurrentThreadWorkerIndex() const
{
	auto It = _ThreadToIndex.find(std::this_thread::get_id());
	return (It != _ThreadToIndex.cend()) ? It->second : std::numeric_limits<decltype(It->second)>().max();
}
//---------------------------------------------------------------------

}
