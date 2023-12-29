#include "Worker.h"
#include <Jobs/JobSystem.h>

namespace DEM::Jobs
{

void CWorker::Init(CJobSystem& Owner, uint32_t Index)
{
	_pOwner = &Owner;
	_Index = Index;
}
//---------------------------------------------------------------------

// FIXME: inline manually?! Not compiled as inline without more headers, but is OK inside templated methods!
void CWorker::PushJob(CJob* pJob)
{
	_Queue.Push(pJob);
	_pOwner->WakeUpWorkers(1); // We have a job now, let's wake up another worker for stealing (e.g. from us)
}
//---------------------------------------------------------------------

void CWorker::DoJob(CJob& Job)
{
	Job.Function();

	//???destroy Function or must destroy it in the same thread where created?

	//???decrement relaxed and publish job results with release only if reached 0?
	//???can increment in AddJob be reordered after this decrement currently? Think of stealing!
	//!!!TODO: C++20 wait on atomic! Or for Windows 8+ can use short spinlock + WaitOnAddress, WakeByAddressSingle and WakeByAddressAll:
	// https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-waitonaddress
	// https://edu.anarcho-copy.org/other/Windows/Windows%2010%20System%20Programming.pdf (search for WaitOnAddress, p.357)
	// https://developers.redhat.com/articles/2022/12/06/implementing-c20-atomic-waiting-libstdc
	// https://rigtorp.se/spinlock/
	// https://probablydance.com/2019/12/30/measuring-mutexes-spinlocks-and-how-bad-the-linux-scheduler-really-is/
	if (auto CounterPtr = Job.Counter.lock())
		if (CounterPtr->fetch_sub(1, std::memory_order_acq_rel) == 1)
			if (auto pJob = _pOwner->EndWaiting(std::move(CounterPtr)))
				PushJob(pJob);
}
//---------------------------------------------------------------------

void CWorker::CancelJob(CJob* pJob)
{
	if (!pJob) return;

	//???destroy Function or must destroy it in the same thread where created?

	// NB: not calling EndWaiting() because now Cancel() is only called from termination
	if (auto CounterPtr = pJob->Counter.lock())
		CounterPtr->fetch_sub(1, std::memory_order_relaxed); // No job results to publish, relaxed is enough
}
//---------------------------------------------------------------------

// TODO: could use fibers to move the current job into a wait list in the middle
// of its execution with CJobSystem::StartWaiting() and continue the main loop without recursion
void CWorker::Wait(std::shared_ptr<std::atomic<uint32_t>> Counter)
{
	if (!Counter) return;
	MainLoop([WaitCounter = std::move(Counter)]() { return WaitCounter->load(std::memory_order_relaxed) == 0; });
	std::atomic_thread_fence(std::memory_order_acquire); // Make finished job results visible
}
//---------------------------------------------------------------------

}
