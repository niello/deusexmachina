#pragma once
#include "WorkStealingQueue.h"
#include <System/Allocators/HalfSafePool.h>
#include <Math/WELL512.h>

// Implements a worker thread logic. After construction, all its fields and methods must be accessed
// from the corresponding worker thread only unless explicitly stated otherwise.

namespace DEM::Jobs
{
class CJobSystem;
using CJobCounter = std::shared_ptr<std::atomic<uint32_t>>; // TODO: make pool and handle manager to use fast and safe handles?

enum EJobType : uint8_t
{
	Normal = 0, // Computational job that uses ~100% of CPU
	Sleepy,     // IO or other waiting job that uses significantly less than 100% of CPU

	Count
};

struct alignas(std::hardware_constructive_interference_size) CJob
{
	std::function<void()> Function;
	CJobCounter           Counter;     // An optional counter decremented on this job completion. External code may wait on it.
	uint8_t               WorkerIndex; // For finding a per-thread pool where the job is created

	template<typename F> CJob(uint8_t WorkerIndex_, F f)
		: Function(std::move(f)), WorkerIndex(WorkerIndex_)
	{}

	template<typename F> CJob(uint8_t WorkerIndex_, F f, const CJobCounter& Counter_)
		: Function(std::move(f)), WorkerIndex(WorkerIndex_), Counter(Counter_)
	{}
};
static_assert(sizeof(CJob) <= alignof(CJob)); // TODO: see what we can do if this asserts. Probably x64 will.

class CWorker final
{
protected:

	CWorkStealingQueue<CJob*> _Queue[EJobType::Count];
	CJobSystem*               _pOwner = nullptr;
	CHalfSafePool<CJob>       _JobPool; //???how to ensure that the job is destroyed by the same pool it was created and from the same thread? Or pool must be lockable/lock-free and so shared (no reason to have per thread then)?
	std::string               _Name;
	uint8_t                   _Index = std::numeric_limits<uint8_t>().max();
	uint8_t                   _JobTypeMask = ~0; //???or initializer list instead of mask?! can tune priorities between types!

	std::mutex                _WaitJobsMutex;
	std::condition_variable   _WaitJobsCV;

	void PushJob(EJobType Type, CJob* pJob);
	void DoJob(CJob& Job);
	void CancelJob(CJob* pJob);

	CJob* PopJob()
	{
		// Search for allowed job in our own queue
		for (uint8_t i = 0; i < EJobType::Count; ++i)
			if (_JobTypeMask & ENUM_MASK(i))
				if (auto pJob = _Queue[i].Pop())
					return pJob;
		return nullptr;
	}

	template<typename F>
	DEM_FORCE_INLINE CJob* AllocateJob(F f)
	{
		static_assert(!std::is_same_v<F, std::nullptr_t>, "Should not pass nullptr as job!");
		return _JobPool.Construct(_Index, std::move(f));
	}

	template<typename F>
	DEM_FORCE_INLINE CJob* AllocateJob(CJobCounter& Counter, F f)
	{
		static_assert(!std::is_same_v<F, std::nullptr_t>, "Should not pass nullptr as job!");

		// Counter must be incremented and assigned to the job before it is pushed to the queue.
		// Otherwise the job may be executed immediately and the counter will never be decremented.
		if (!Counter)
			Counter.reset(new std::atomic<uint32_t>(1));
		else
			Counter->fetch_add(1, std::memory_order_relaxed);

		return _JobPool.Construct(_Index, std::move(f), Counter);
	}

	// Implements https://taskflow.github.io/taskflow/icpads20.pdf with some changes
	template<typename TPred>
	void MainLoop(TPred ExitPred)
	{
		//!!!DBG TMP!
		ZoneScopedN("DBG THREAD LOOP");

		const size_t ThreadCount = _pOwner->GetWorkerThreadCount();
		const size_t MaxStealsBeforeYield = 2 * (ThreadCount + 1);
		const size_t MaxStealAttempts = MaxStealsBeforeYield * 64;

		// PERF: WELL512 is slightly faster in my local tests
		//std::default_random_engine VictimRNG{ std::random_device{}() };
		Math::CWELL512 VictimRNG{ std::random_device{}() };
		std::uniform_int_distribution<size_t> GetRandomVictim(0, ThreadCount - 2); // Exclude the current worker from the range, see generation below
		size_t Victim = ThreadCount; // Start stealing from the main thread

		// Main loop of the worker thread implements a state-machine of 3 states: local queue loop, stealing loop and sleeping.
		while (true)
		{
			// Process the local queue until it is empty or until termination is requested
			while (true)
			{
				if (ExitPred()) return;

				CJob* pJob = PopJob();

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

					pJob = _pOwner->GetWorker(static_cast<uint8_t>(Victim)).Steal(_JobTypeMask);

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
					for (uint8_t i = 0; i <= ThreadCount; ++i)
					{
						if (i == _Index) continue;
						pJob = _pOwner->GetWorker(i).Steal(_JobTypeMask);
						if (pJob) break;
					}
				}

				if (pJob)
				{
					// We have stolen a job an will be busy, wake up one more worker to continue stealing jobs
					_pOwner->WakeUpWorker();

					// Do the job and return to the local queue loop because this job might push new jobs to it
					DoJob(*pJob);
					break;
				}
				else
				{
					// This store must not be reordered past the sleep condition evaluation. Otherwise a condition may
					// be evaluated to true, then WakeUp() will preempt us, read "waiting is false" and skip notification.
					// This thread will resume and start waiting on CV. This results in a missing wakeup. Making sure that the
					// waiting flag is set before eliminates this case. We either see "waiting is true" and send notification
					// or we skip notification due to "waiting is false" but sleep condition will detect new jobs, if any.
					_pOwner->SetWorkerWaitingJob(_Index);

					// No jobs to steal, go to sleep. After waking up the worker returns to stealing because no one could push jobs into its local queue.
					// TODO PERF C++20: wait on atomic?!
					bool NeedExit = false;
					{
						std::unique_lock Lock(_WaitJobsMutex);

						NeedExit = ExitPred() || _pOwner->IsTerminationRequested(true);
						while (!NeedExit && !_pOwner->HasJobs(_JobTypeMask))
						{
							_WaitJobsCV.wait(Lock);
							NeedExit = ExitPred() || _pOwner->IsTerminationRequested(true);
						}

						_pOwner->SetWorkerNotWaitingJob(_Index);
					}

					// We could have been woken up because of termination request, let's check immediately
					if (NeedExit) return;

					// We don't know who has sent a signal, start stealing from the main thread.
					// This is a good choice because the main thread is the most likely to have new jobs.
					Victim = ThreadCount;
				}
			}
		}
	}

public:

	void Init(CJobSystem& Owner, std::string Name, uint8_t Index, uint8_t JobTypeMask = ~0);
	void MainLoop() { MainLoop([]() { return false; }); }
	void WaitActive(CJobCounter Counter);
	void WaitIdle(CJobCounter Counter);

	// Shortcuts for normal jobs
	template<typename F> DEM_FORCE_INLINE void AddJob(F f) { AddJob(EJobType::Normal, f); }
	template<typename F> DEM_FORCE_INLINE void AddJob(CJobCounter& Counter, F f) { AddJob(EJobType::Normal, Counter, f); }
	template<typename F> DEM_FORCE_INLINE void AddWaitingJob(CJobCounter WaitCounter, F f) { AddWaitingJob(EJobType::Normal, std::move(WaitCounter), f); }
	template<typename F> DEM_FORCE_INLINE void AddWaitingJob(CJobCounter& Counter, CJobCounter WaitCounter, F f) { AddWaitingJob(EJobType::Normal, Counter, std::move(WaitCounter), f); }

	template<typename F>
	DEM_FORCE_INLINE void AddJob(EJobType Type, F f)
	{
		PushJob(Type, AllocateJob(f));
	}

	template<typename F>
	DEM_FORCE_INLINE void AddJob(EJobType Type, CJobCounter& Counter, F f)
	{
		PushJob(Type, AllocateJob(Counter, f));
	}

	template<typename F>
	DEM_FORCE_INLINE void AddWaitingJob(EJobType Type, CJobCounter WaitCounter, F f)
	{
		CJob* pJob = AllocateJob(f);
		if (!_pOwner->StartWaiting(std::move(WaitCounter), pJob, Type))
			PushJob(Type, pJob);
	}

	template<typename F>
	DEM_FORCE_INLINE void AddWaitingJob(EJobType Type, CJobCounter& Counter, CJobCounter WaitCounter, F f)
	{
		CJob* pJob = AllocateJob(Counter, f);
		if (!_pOwner->StartWaiting(std::move(WaitCounter), pJob, Type))
			PushJob(Type, pJob);
	}

	void Push(EJobType Type, CJob* pJob) { return _Queue[Type].Push(pJob); }

	// Can be called from any thread
	CJob* Steal(uint8_t TypeMask)
	{
		// Search for allowed job in our own queue
		for (uint8_t i = 0; i < EJobType::Count; ++i)
			if (TypeMask & ENUM_MASK(i))
				if (auto pJob = _Queue[i].Steal())
					return pJob;
		return nullptr;
	}

	// Can be called from any thread
	bool HasJobs(uint8_t TypeMask = ~0) const
	{
		for (uint8_t i = 0; i < EJobType::Count; ++i)
			if ((TypeMask & ENUM_MASK(i)) && !_Queue[i].empty()) return true;
		return false;
	}

	// Can be called from any thread
	uint8_t CollectAvailableJobsMask() const
	{
		uint8_t Mask = 0;
		for (uint8_t i = 0; i < EJobType::Count; ++i)
			if (!_Queue[i].empty()) Mask |= ENUM_MASK(i);
		return Mask;
	}

	// Can be called from any thread
	bool WakeUp();

	const std::string& GetName() const { return _Name; }
	uint8_t            GetIndex() const { return _Index; }
	uint8_t            GetJobTypeMask() const { return _JobTypeMask; }
};

}
