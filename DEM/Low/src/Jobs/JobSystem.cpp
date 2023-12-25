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

	_Workers.reserve(ThreadCount);

	uint32_t ThreadsStarted = 0;
	std::mutex ThreadsStartedMutex;
	std::condition_variable ThreadsStartedCV;

	//!!!FIXME: use fmtlib!
	const std::string ThreadNamePrefixStr(ThreadNamePrefix);

	for (uint32_t i = 0; i < ThreadCount; ++i)
	{
		_Workers.emplace_back(*this, i, std::thread([this, i, ThreadCount, &ThreadNamePrefixStr, &ThreadsStarted, &ThreadsStartedMutex, &ThreadsStartedCV]
		{
			Sys::SetCurrentThreadName(ThreadNamePrefixStr + std::to_string(i));

			// Let the system know that we are ready to go. Wake up the constructor thread if we were the last.
			{
				std::lock_guard Lock(ThreadsStartedMutex);
				if (++ThreadsStarted == ThreadCount) ThreadsStartedCV.notify_one();
			}

			//!!!DBG TMP!
			ZoneScopedN("THREAD");
			using namespace std::chrono_literals;
			std::this_thread::sleep_for(1000ms);

			// run worker loop: while (pThisWorker->MainLoop()) ;
		}));
	}

	// Wait for all worker threads to start
	{
		std::unique_lock Lock(ThreadsStartedMutex);
		ThreadsStartedCV.wait(Lock, [&ThreadsStarted, ThreadCount]() { return ThreadsStarted == ThreadCount; });
	}
}

}
