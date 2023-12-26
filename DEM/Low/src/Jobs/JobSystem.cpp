#include "JobSystem.h"
#include <System/System.h>
#include <StdDEM.h>
#include <string>
#include <mutex>
#include <condition_variable>

namespace DEM::Jobs
{

CJobSystem::CJobSystem(uint32_t ThreadCount, std::string_view ThreadNamePrefix)
{
	if (ThreadCount < 1) ThreadCount = 1;

	// The last worker is for the main thread, this simplifies a design
	_Workers.reset(new CWorker[ThreadCount + 1]);
	for (uint32_t i = 0; i <= ThreadCount; ++i)
		_Workers[i].Init(*this, i);

	_ThreadToIndex.emplace(std::this_thread::get_id(), ThreadCount);

	uint32_t ThreadsStarted = 0;
	std::mutex ThreadsStartedMutex;
	std::condition_variable ThreadsStartedCV;

	//!!!FIXME: use fmtlib!
	const std::string ThreadNamePrefixStr(ThreadNamePrefix);

	_Threads.reserve(ThreadCount);
	for (uint32_t i = 0; i < ThreadCount; ++i)
	{
		_Threads.emplace_back([this, i, ThreadCount, &ThreadNamePrefixStr, &ThreadsStarted, &ThreadsStartedMutex, &ThreadsStartedCV]
		{
			Sys::SetCurrentThreadName(ThreadNamePrefixStr + std::to_string(i));

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

CJobSystem::~CJobSystem()
{
	//???wait for all jobs or cancel? either should call WaitAll() or Cancel() explicitly at the app exit if wants different behaviour!
	//???need global job counter for WaitAll?

	// Set a termination flag for workers to see the request
	_TerminationRequested.store(true, std::memory_order_relaxed);

	// Wake up all sleeping workers so that they can terminate properly
	//{
	//	std::lock_guard Lock(WaitJobsMutex);
	//	WaitJobsCV.notify_all();
	//}

	// Wait for all worker threads to terminate
	for (auto& Thread: _Threads)
		if (Thread.joinable()) Thread.join();
}
//---------------------------------------------------------------------

}
