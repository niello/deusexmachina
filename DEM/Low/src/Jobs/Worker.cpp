#include "Worker.h"
#include <Jobs/JobSystem.h>

namespace DEM::Jobs
{

void CWorker::Init(CJobSystem& Owner, std::string Name, uint8_t Index, uint8_t JobTypeMask)
{
	_pOwner = &Owner;
	_Name = std::move(Name);
	_Index = Index;
	_JobTypeMask = JobTypeMask;
}
//---------------------------------------------------------------------

// FIXME: inline manually?! Not compiled as inline without more headers, but is OK inside templated methods!
void CWorker::PushJob(EJobType Type, CJob* pJob)
{
	_Queue[Type].Push(pJob);
	_pOwner->WakeUpWorker(ENUM_MASK(Type)); // We have a job now, let's wake up another worker for stealing (e.g. from us)
}
//---------------------------------------------------------------------

void CWorker::DoJob(CJob& Job)
{
	Job.Function();

	//???decrement relaxed and publish job results with release only if reached 0?
	//???can increment in AddJob be reordered after this decrement currently? Think of stealing!
	//!!!TODO: C++20 wait on atomic! Or for Windows 8+ can use short spinlock + WaitOnAddress, WakeByAddressSingle and WakeByAddressAll:
	// https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-waitonaddress
	// https://edu.anarcho-copy.org/other/Windows/Windows%2010%20System%20Programming.pdf (search for WaitOnAddress, p.357)
	// https://developers.redhat.com/articles/2022/12/06/implementing-c20-atomic-waiting-libstdc
	// https://rigtorp.se/spinlock/
	// https://probablydance.com/2019/12/30/measuring-mutexes-spinlocks-and-how-bad-the-linux-scheduler-really-is/
	if (auto pCounter = Job.Counter.get())
		if (pCounter->fetch_sub(1, std::memory_order_acq_rel) == 1)
			_pOwner->EndWaiting(std::move(Job.Counter), *this);

	_pOwner->GetWorker(Job.WorkerIndex)._JobPool.Destroy(&Job);
}
//---------------------------------------------------------------------

void CWorker::CancelJob(CJob* pJob)
{
	if (!pJob) return;

	// NB: not calling EndWaiting() because now CancelJob() is only called from termination
	if (auto pCounter = pJob->Counter.get())
		pCounter->fetch_sub(1, std::memory_order_relaxed); // No job results to publish, relaxed is enough

	_pOwner->GetWorker(pJob->WorkerIndex)._JobPool.Destroy(pJob);
}
//---------------------------------------------------------------------

// Active waiting. The worker thread is allowed to pick and execute independent jobs while waiting on the counter.
// TODO: could use fibers to move the current job into a wait list in the middle
// of its execution with CJobSystem::StartWaiting() and continue the main loop without recursion
void CWorker::WaitActive(CJobCounter Counter)
{
	if (!_pOwner->StartWaiting(Counter, _Index)) return;

	MainLoop([WaitCounter = std::move(Counter)]() { return WaitCounter->load(std::memory_order_relaxed) == 0; });

	// Make finished job results visible
	std::atomic_thread_fence(std::memory_order_acquire);
}
//---------------------------------------------------------------------

// Inactive waiting. The worker thread yelds until the counter reaches zero.
void CWorker::WaitIdle(CJobCounter Counter)
{
	if (!_pOwner->StartWaiting(Counter, _Index)) return;

	// Waiting logic is the same as in MainLoop, see comments there for details
	_pOwner->SetWorkerSleeping(_Index);

	// Lock also works as an acquire fence, making job results from Counter visible when the counter reaches zero
	// TODO PERF C++20: wait on atomic?!
	std::unique_lock Lock(_WaitJobsMutex);
	while (Counter->load(std::memory_order_relaxed) != 0 && !_pOwner->IsTerminationRequested(true))
		_WaitJobsCV.wait(Lock);

	_pOwner->SetWorkerAwakened(_Index);
}
//---------------------------------------------------------------------

// Can be called from any thread
bool CWorker::WakeUp()
{
	std::lock_guard Lock(_WaitJobsMutex);
	if (!_pOwner->IsWorkerSleeping(_Index)) return false;
	_WaitJobsCV.notify_one();
	return true;
}
//---------------------------------------------------------------------

}
