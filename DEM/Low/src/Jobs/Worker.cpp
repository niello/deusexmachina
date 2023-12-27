#include "Worker.h"
#include <Jobs/JobSystem.h>
#include <random>

namespace DEM::Jobs
{

void CWorker::Init(CJobSystem& Owner, uint32_t Index)
{
	_pOwner = &Owner;
	_Index = Index;
}
//---------------------------------------------------------------------

// Implements https://taskflow.github.io/taskflow/icpads20.pdf with some changes
void CWorker::MainLoop()
{
	//!!!DBG TMP!
	ZoneScopedN("DBG THREAD LOOP");

	const size_t MaxStealsBeforeYield = 2 * (_pOwner->GetWorkerThreadCount() + 1);
	const size_t MaxStealAttempts = MaxStealsBeforeYield * 64;

	// TODO: use own WELL512?
	std::default_random_engine VictimRNG{ std::random_device{}() };
	std::uniform_int_distribution<size_t> GetRandomVictim(0, _pOwner->GetWorkerThreadCount() - 2); // Exclude the current worker from the range, see generation below
	size_t Victim = _pOwner->GetWorkerThreadCount(); // Start stealing from the main thread

	// Main loop of the worker thread implements a state-machine of 3 states: local queue loop, stealing loop and sleeping.
	while (true)
	{
		// Process the local queue until it is empty or until termination is requested
		while (true)
		{
			auto pJob = _Queue.Pop();
			if (_pOwner->IsTerminationRequested() /*|| (Counter && *Counter == 0) || ProcessedJobLimitExceeded*/) return;
			if (!pJob) break;
			pJob->Function();
		}

		// Try stealing from random victims
		while (true)
		{
			CJob* pJob = nullptr;
			size_t StealsWithoutYield = 0;
			for (size_t StealAttempts = 0; StealAttempts < MaxStealAttempts; ++StealAttempts)
			{
				pJob = _pOwner->GetWorker(Victim).Steal();

				if (_pOwner->IsTerminationRequested() /*|| (Counter && *Counter == 0) || ProcessedJobLimitExceeded*/) return;

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
				for (size_t i = 0; i <= _pOwner->GetWorkerThreadCount(); ++i)
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
				pJob->Function();
				break;
			}
			else
			{
				// No jobs to steal, go to sleep. After waking up the worker returns to the stealing loop because no one could push jobs into its local queue.
				_pOwner->PutCurrentWorkerToSleepUntil([this] { return _pOwner->IsTerminationRequested() || _pOwner->HasJobs(); });

				// We could have been woken up because of termination request, let's check immediately
				if (_pOwner->IsTerminationRequested() /*|| (Counter && *Counter == 0) || ProcessedJobLimitExceeded*/) return;

				// We don't know who has sent a signal, start stealing from the main thread.
				// This is a good choice because the main thread is the most likely to have new jobs.
				Victim = _pOwner->GetWorkerThreadCount();
			}
		}
	}
}
//---------------------------------------------------------------------

}
