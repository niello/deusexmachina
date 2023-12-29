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

// TODO: could use fibers to move the current job into a wait list in the middle
// of its execution and continue the main loop without recursion
void CWorker::Wait(std::shared_ptr<std::atomic<uint32_t>> Counter)
{
	if (!Counter) return;
	MainLoop([WaitCounter = std::move(Counter)]() { return WaitCounter->load(std::memory_order_relaxed) == 0; });
	std::atomic_thread_fence(std::memory_order_acquire); // Make finished job results visible
}
//---------------------------------------------------------------------

}
