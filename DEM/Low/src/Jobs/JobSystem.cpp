#include "JobSystem.h"
#include <System/System.h>
#include <StdDEM.h>
#include <string>
#include <mutex>
#include <condition_variable>

namespace DEM::Jobs
{

CJobSystem::CJobSystem(bool Sleepy, uint32_t ThreadCount, std::string_view ThreadNamePrefix)
{
	if (ThreadCount < 1) ThreadCount = 1;

	// The last worker is for the main thread, this simplifies a design
	_Workers.reset(new CWorker[ThreadCount + 1]);
	for (uint32_t i = 0; i <= ThreadCount; ++i)
		_Workers[i].Init(*this, i);

	// NB: it is crucial not to reserve but to resize the vector because we use its
	// size as a thread count and access it in workers before we create all threads
	_Threads.resize(ThreadCount);

	_ThreadToIndex.emplace(std::this_thread::get_id(), ThreadCount);

	uint32_t ThreadsStarted = 0;
	std::mutex ThreadsStartedMutex;
	std::condition_variable ThreadsStartedCV;

	//!!!FIXME: use fmtlib!
	const std::string ThreadNamePrefixStr(ThreadNamePrefix);

	for (uint32_t i = 0; i < ThreadCount; ++i)
	{
		_Threads[i] = std::thread([this, i, Sleepy, ThreadCount, &ThreadNamePrefixStr, &ThreadsStarted, &ThreadsStartedMutex, &ThreadsStartedCV]
		{
			const std::string ThreadName = ThreadNamePrefixStr + std::to_string(i);
			Sys::SetCurrentThreadName(ThreadName);
			tracy::SetThreadName(ThreadName.c_str());

			// NB: sleepy jobs (IO, network etc) don't need an affinity because they are blocked most of the time and we want them to wake anywhere possible
			// TODO PERF: test under the real workload. Profiling shows that this may perform better or worse depending on the test itself.
			if (!Sleepy) Sys::SetCurrentThreadAffinity(i);

			//???!!!TODO: raise thread priority for sleepy jobs to fight priority inversion, when some normal job waits dynamically spawned IO job?!

			// Let the system know that we are ready to go. Wake up the constructor thread if we were the last.
			{
				std::lock_guard Lock(ThreadsStartedMutex);
				_ThreadToIndex.emplace(std::this_thread::get_id(), i);
				if (++ThreadsStarted == ThreadCount) ThreadsStartedCV.notify_one();
			}

			auto& ThisWorker = _Workers[i];
			ThisWorker.MainLoop();
		});
	}

	// Wait for all worker threads to start
	{
		std::unique_lock Lock(ThreadsStartedMutex);
		ThreadsStartedCV.wait(Lock, [&ThreadsStarted, ThreadCount]() { return ThreadsStarted == ThreadCount; });
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

bool CJobSystem::StartWaiting(std::shared_ptr<std::atomic<uint32_t>> Counter, CJob* pJob)
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

		auto It = _WaitList.find(Counter);
		if (It == _WaitList.cend())
		{
			It = _WaitList.emplace(std::move(Counter), pJob).first;
		}
		else
		{
			//???build chain of counters?! can get counter from job, or create if it hasn't. But this will serialize!
			NOT_IMPLEMENTED;
		}
	}

	// The job is successfully added to the wait list and will be found by the next EndWaiting()
	return true;
}
//---------------------------------------------------------------------

CJob* CJobSystem::EndWaiting(std::shared_ptr<std::atomic<uint32_t>> Counter)
{
	// Assume the counter being 0, otherwise this method wouldn't be called.
	// Incrementing and starting waiting on the same counter is illegal until it is removed from the wait list.

	CJob* pJob = nullptr;
	{
		std::unique_lock Lock(_WaitListMutex);
		auto It = _WaitList.find(Counter);
		if (It != _WaitList.cend())
		{
			pJob = It->second;
			_WaitList.erase(It);
		}
	}
	return pJob;
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
