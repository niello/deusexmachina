#pragma once
#include "WorkStealingQueue.h"
#include <System/Allocators/PoolAllocator.h>

// Implements a worker thread logic. After construction, all its fields are intended for access from the corresponding thread only.

namespace DEM::Jobs
{
class CJobSystem;

//!!!DBG TMP!
struct CJob
{
	std::function<void()>              Function;
	std::weak_ptr<std::atomic<size_t>> Counter;

	DEM_FORCE_INLINE void operator()()
	{
		Function();

		//!!!TODO: if reached 0, notify!
		//!!!TODO: C++20 wait on atomic!
		if (auto CounterPtr = Counter.lock())
			CounterPtr->fetch_sub(1, std::memory_order_acq_rel);
	}
};

class CWorker final
{
protected:

	inline static constexpr auto JOB_ALIGN = std::max<size_t>(alignof(CJob), std::hardware_constructive_interference_size);

	// CJob - to CJobSystem or separate header? Job = dependency counter + std::function. Or store counter only in a wait list?
	// also job must store a list of counters (or always a single one?) which it must decrement when it is finished.

	//Random generator
	//mutex and conditional variable for sleeping when there are no jobs; or a single cond var in a CJobSystem?! local queue can't be pushed in when the thread sleeps.
	//???wait list for jobs with dependencies? need pool allocator for waiting nodes. Or wait list must be global?

	CWorkStealingQueue<CJob*> _Queue;
	CJobSystem*               _pOwner = nullptr;
	CPool<CJob, JOB_ALIGN>    _JobPool; //???how to ensure that the job is destroyed by the same pool it was created and from the same thread? Or pool must be global and lockable?
	uint32_t                  _Index = std::numeric_limits<uint32_t>().max();

	template<typename F>
	void AddJobInternal(F f, const std::shared_ptr<std::atomic<size_t>>& Counter)
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
	DEM_FORCE_INLINE void AddJob(std::shared_ptr<std::atomic<size_t>>& Counter, F f)
	{
		// Counter must be incremented and assigned to the job before it is pushed to the queue.
		// Otherwise the job may be executed immediately and the counter will never be decremented.
		if (!Counter)
			Counter.reset(new std::atomic<size_t>(1));
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
