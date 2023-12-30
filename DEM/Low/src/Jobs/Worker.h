#pragma once
#include "WorkStealingQueue.h"
#include <System/Allocators/PoolAllocator.h>
#include <random>

// Implements a worker thread logic. After construction, all its fields are intended for access from the corresponding thread only.

namespace DEM::Jobs
{
class CJobSystem;

struct alignas(std::hardware_constructive_interference_size) CJob
{
	std::function<void()>                Function;
	std::weak_ptr<std::atomic<uint32_t>> Counter;  // An optional counter decremented on this job completion. External code may wait on it.
};
static_assert(sizeof(CJob) <= alignof(CJob)); // TODO: see what we can do if this asserts. Probably x64 will.

class CWorker final
{
protected:

	CWorkStealingQueue<CJob*> _Queue;
	CJobSystem*               _pOwner = nullptr;
	CPool<CJob>               _JobPool; //???how to ensure that the job is destroyed by the same pool it was created and from the same thread? Or pool must be lockable/lock-free and so shared (no reason to have per thread then)?
	uint32_t                  _Index = std::numeric_limits<uint32_t>().max();

	void PushJob(CJob* pJob);
	void DoJob(CJob& Job);
	void CancelJob(CJob* pJob);

	// Implements https://taskflow.github.io/taskflow/icpads20.pdf with some changes
	template<typename TPred>
	void MainLoop(TPred ExitPred)
	{
		//!!!DBG TMP!
		ZoneScopedN("DBG THREAD LOOP");

		const size_t ThreadCount = _pOwner->GetWorkerThreadCount();
		const size_t MaxStealsBeforeYield = 2 * (ThreadCount + 1);
		const size_t MaxStealAttempts = MaxStealsBeforeYield * 64;

		// TODO: use own WELL512?
		std::default_random_engine VictimRNG{ std::random_device{}() };
		std::uniform_int_distribution<size_t> GetRandomVictim(0, ThreadCount - 2); // Exclude the current worker from the range, see generation below
		size_t Victim = ThreadCount; // Start stealing from the main thread

		// Main loop of the worker thread implements a state-machine of 3 states: local queue loop, stealing loop and sleeping.
		while (true)
		{
			// Process the local queue until it is empty or until termination is requested
			while (true)
			{
				if (ExitPred()) return;

				auto pJob = _Queue.Pop();

				if (_pOwner->IsTerminationRequested())
				{
					CancelJob(pJob);
					return;
				}

				if (!pJob) break;

				DoJob(*pJob);
			}

			// Try stealing from random victims
			while (true)
			{
				CJob* pJob = nullptr;
				size_t StealsWithoutYield = 0;
				for (size_t StealAttempts = 0; StealAttempts < MaxStealAttempts; ++StealAttempts)
				{
					if (ExitPred()) return;

					pJob = _pOwner->GetWorker(Victim).Steal();

					if (_pOwner->IsTerminationRequested())
					{
						CancelJob(pJob);
						return;
					}

					if (pJob) break;

					if (++StealsWithoutYield >= MaxStealsBeforeYield)
					{
						StealsWithoutYield = 0;
						std::this_thread::yield();
					}

					// Steal attempt to the current victim has failed, try another one. Skip our index.
					Victim = GetRandomVictim(VictimRNG);
					if (Victim >= _Index) ++Victim;
				}

				// There is a big chance that randomization will not return us the index of the thread that has jobs to steal.
				// As a last resort, try to scan all workers including a main thread worker. This is especially helpful when there are many workers.
				if (!pJob)
				{
					// TODO: start from the main thread?
					for (size_t i = 0; i <= ThreadCount; ++i)
					{
						if (i == _Index) continue;
						pJob = _pOwner->GetWorker(i).Steal();
						if (pJob) break;
					}
				}

				if (pJob)
				{
					// We have stolen a job an will be busy, wake up one more worker to continue stealing jobs
					_pOwner->WakeUpWorkers(1);

					// Do the job and return to the local queue loop because this job might push new jobs to it
					DoJob(*pJob);
					break;
				}
				else
				{
					// No jobs to steal, go to sleep. After waking up the worker returns to the stealing loop because no one could push jobs into its local queue.
					_pOwner->PutCurrentWorkerToSleepUntil([this, &ExitPred] { return _pOwner->IsTerminationRequested() || ExitPred() || _pOwner->HasJobs(); });

					// We could have been woken up because of termination request, let's check immediately
					if (_pOwner->IsTerminationRequested() || ExitPred()) return;

					// We don't know who has sent a signal, start stealing from the main thread.
					// This is a good choice because the main thread is the most likely to have new jobs.
					Victim = ThreadCount;
				}
			}
		}
	}

public:

	void Init(CJobSystem& Owner, uint32_t Index);
	void MainLoop() { MainLoop([]() { return false; }); }
	void Wait(std::shared_ptr<std::atomic<uint32_t>> Counter);

	template<typename F>
	DEM_FORCE_INLINE void AddJob(F f)
	{
		//!!!DBG TMP!
		//!!!FIXME: leak!!!
		CJob* pJob = _JobPool.Construct();
		pJob->Function = std::move(f); //???can piecewise construct be useful here? pass f right into _JobPool.Construct()?
		pJob->Counter.reset();
		PushJob(pJob);
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

		//!!!DBG TMP!
		//!!!FIXME: leak!!!
		CJob* pJob = _JobPool.Construct();
		pJob->Function = std::move(f); //???can piecewise construct be useful here? pass f right into _JobPool.Construct()?
		pJob->Counter = Counter;
		PushJob(pJob);
	}

	template<typename F>
	DEM_FORCE_INLINE void AddWaitingJob(std::shared_ptr<std::atomic<uint32_t>> WaitCounter, F f)
	{
		//!!!DBG TMP!
		//!!!FIXME: leak!!!
		CJob* pJob = _JobPool.Construct();
		pJob->Function = std::move(f); //???can piecewise construct be useful here? pass f right into _JobPool.Construct()?
		pJob->Counter.reset();

		if (!_pOwner->StartWaiting(WaitCounter, pJob))
			PushJob(pJob);
	}

	template<typename F>
	DEM_FORCE_INLINE void AddWaitingJob(std::shared_ptr<std::atomic<uint32_t>>& Counter, std::shared_ptr<std::atomic<uint32_t>> WaitCounter, F f)
	{
		// Counter must be incremented and assigned to the job before it is pushed to the queue.
		// Otherwise the job may be executed immediately and the counter will never be decremented.
		if (!Counter)
			Counter.reset(new std::atomic<uint32_t>(1));
		else
			Counter->fetch_add(1, std::memory_order_relaxed);

		//!!!DBG TMP!
		//!!!FIXME: leak!!!
		CJob* pJob = _JobPool.Construct();
		pJob->Function = std::move(f); //???can piecewise construct be useful here? pass f right into _JobPool.Construct()?
		pJob->Counter = Counter;

		if (!_pOwner->StartWaiting(WaitCounter, pJob))
			PushJob(pJob);
	}

	void  Push(CJob* pJob) { return _Queue.Push(pJob); }
	CJob* Steal() { return _Queue.Steal(); }

	auto  GetIndex() const { return _Index; }
	bool  HasJobs() const { return !_Queue.empty(); }

	//???or in a CJobSystem, by index?
	// SetCapabilities (mask - main thread, render context etc)
};

}
