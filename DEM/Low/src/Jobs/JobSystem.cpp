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
	_TerminationRequested.store(true, std::memory_order_relaxed);

	// Wake up all sleeping workers so that they can terminate properly
	WakeUpAllWorkers();

	// Wait for all worker threads to terminate
	for (auto& Thread: _Threads)
		if (Thread.joinable()) Thread.join();
}
//---------------------------------------------------------------------

bool CJobSystem::StartWaiting(CJobCounter Counter, CJob* pJob, EJobType JobType)
{
	// There are no unsatisfied dependencies, return false to let the worker enqueue the job immediately
	if (!Counter || Counter->load(std::memory_order_relaxed) == 0) return false;

	{
		std::unique_lock Lock(_WaitListMutex);

		// Counter might have changed after the previous check but before we locked the mutex, check again.
		// This check will not be reordered before the lock because locking has acquire semantics.
		// Return false to let the worker enqueue the job immediately. EndWaiting() has either finished or was
		// not called yet or is waiting for the _WaitListMutex. In any case it will not find pJob in the wait list.
		if (Counter->load(std::memory_order_relaxed) == 0) return false;

		// Only workers can call StartWaiting() and they never push the same job twice, no duplicate check is needed
		_WaitList.emplace(std::move(Counter), std::make_pair(pJob, JobType));
	}

	// The job is successfully added to the wait list and will be found by the next EndWaiting()
	return true;
}
//---------------------------------------------------------------------

void CJobSystem::EndWaiting(CJobCounter Counter, CWorker& Worker)
{
	// Assume the counter being 0, otherwise this method wouldn't be called.
	// Incrementing and starting waiting on the same counter is illegal until it is removed from the wait list.

	size_t NewJobCount = 0;
	{
		std::unique_lock Lock(_WaitListMutex);
		auto [ItBegin, ItEnd] = _WaitList.equal_range(Counter);
		if (ItBegin == ItEnd) return;

		// TODO: pushing jobs should not be protected by mutex but it is probably better than copying job pointers
		// to an intermediate array. If will rewrite to linked list of waiting jobs, can copy list head.
		for (auto It = ItBegin; It != ItEnd; ++It, ++NewJobCount)
			Worker.Push(It->second.second, It->second.first);

		_WaitList.erase(ItBegin, ItEnd);
	}

	WakeUpWorkers(NewJobCount);
}
//---------------------------------------------------------------------

void CJobSystem::WakeUpWorkers(size_t Count)
{
	if (Count >= _Threads.size())
	{
		WakeUpAllWorkers();
	}
	else
	{
		std::lock_guard Lock(_WaitJobsMutex);
		for (size_t i = 0; i < Count; ++i)
			_WaitJobsCV.notify_one();
	}
}
//---------------------------------------------------------------------

void CJobSystem::WakeUpAllWorkers()
{
	std::lock_guard Lock(_WaitJobsMutex);
	_WaitJobsCV.notify_all();
}
//---------------------------------------------------------------------

bool CJobSystem::HasJobs() const
{
	const auto ThreadCount = _Threads.size();
	for (uint32_t i = 0; i <= ThreadCount; ++i)
		if (_Workers[i].HasJobs()) return true;
	return false;
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
