#pragma once
#include "WorkStealingQueue.h"
#include <System/Allocators/PoolAllocator.h>

// Implements a worker thread logic. After construction, all its fields are intended for access from the corresponding thread only.

namespace DEM::Jobs
{
class CJobSystem;

struct alignas(std::hardware_constructive_interference_size) CJob
{
	std::function<void()>                Function;
	std::weak_ptr<std::atomic<uint32_t>> Counter;

	DEM_FORCE_INLINE void operator()()
	{
		Function();

		//???!!!should be protected?! what if counter is destroyed in another thread? what if count is incremented concurrently here?
		//!!!TODO: if reached 0, notify waiters!
		//???decrement relaxed and publish job results with release only if reached 0?
		//???can increment in AddJob be reordered after this decrement currently? Think of stealing!
		//!!!TODO: C++20 wait on atomic! Or for Windows 8+ can use short spinlock + WaitOnAddress, WakeByAddressSingle and WakeByAddressAll:
		// https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-waitonaddress
		// https://edu.anarcho-copy.org/other/Windows/Windows%2010%20System%20Programming.pdf (search for WaitOnAddress, p.357)
		// https://developers.redhat.com/articles/2022/12/06/implementing-c20-atomic-waiting-libstdc
		// https://rigtorp.se/spinlock/
		// https://probablydance.com/2019/12/30/measuring-mutexes-spinlocks-and-how-bad-the-linux-scheduler-really-is/
		if (auto CounterPtr = Counter.lock())
			CounterPtr->fetch_sub(1, std::memory_order_acq_rel);
	}
};
static_assert(sizeof(CJob) <= alignof(CJob)); // TODO: see what we can do if this asserts. Probably x64 will.

class CWorker final
{
protected:

	// CJob - to CJobSystem or separate header? Job = dependency counter + std::function. Or store counter only in a wait list?
	// also job must store a list of counters (or always a single one?) which it must decrement when it is finished.

	//Random generator
	//mutex and conditional variable for sleeping when there are no jobs; or a single cond var in a CJobSystem?! local queue can't be pushed in when the thread sleeps.
	//???wait list for jobs with dependencies? need pool allocator for waiting nodes. Or wait list must be global?

	CWorkStealingQueue<CJob*> _Queue;
	CJobSystem*               _pOwner = nullptr;
	CPool<CJob>               _JobPool; //???how to ensure that the job is destroyed by the same pool it was created and from the same thread? Or pool must be global and lockable?
	uint32_t                  _Index = std::numeric_limits<uint32_t>().max();

	template<typename F>
	void AddJobInternal(F f, const std::shared_ptr<std::atomic<uint32_t>>& Counter)
	{
		//!!!DBG TMP!
		//!!!FIXME: leak!!!
		CJob* pJob = _JobPool.Construct();
		pJob->Function = std::move(f);
		pJob->Counter = Counter;
		_Queue.Push(pJob);

		// We have a job now, let's wake up another worker for stealing
		_pOwner->WakeUpWorkers(1);
	}

public:

	void Init(CJobSystem& Owner, uint32_t Index);
	void MainLoop();

	template<typename F>
	DEM_FORCE_INLINE void AddJob(F f)
	{
		AddJobInternal(f, nullptr);
	}

	template<typename F>
	DEM_FORCE_INLINE void AddJob(std::shared_ptr<std::atomic<uint32_t>>& Counter, F f)
	{
		// Counter must be incremented and assigned to the job before it is pushed to the queue.
		// Otherwise the job may be executed immediately and the counter will never be decremented.
		if (!Counter)
			Counter.reset(new std::atomic<uint32_t>(1));
		else
			Counter->fetch_add(1, std::memory_order_relaxed);

		AddJobInternal(f, Counter);
	}

	CJob* Steal() { return _Queue.Steal(); }

	bool  HasJobs() const { return !_Queue.empty(); }

	//???or in a CJobSystem, by index?
	// SetCapabilities (mask - main thread, render context etc)
};

}
