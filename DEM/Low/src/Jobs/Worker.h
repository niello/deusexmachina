#pragma once
#include "WorkStealingQueue.h"

// Implements a worker thread logic. After construction, all its fields are intended for access from the corresponding thread only.

namespace DEM::Jobs
{
class CJobSystem;

class CWorker final
{
protected:

	// CJob - to CJobSystem or separate header? Job = dependency counter + std::function. Or store counter only in a wait list?
	// also job must store a list of counters (or always a single one?) which it must decrement when it is finished.

	//CWorkStealingQueue<CJob*>
	//CPoolAllocator<CJob> - ensure works well when the job is stolen, how to free job in the allocation thread, not in stealing thread?! Allocate aligned by cacheline.
	//Random generator
	//mutex and conditional variable for sleeping when there are no jobs; or a single cond var in a CJobSystem?! local queue can't be pushed in when the thread sleeps.
	//???wait list for jobs with dependencies? need pool allocator for waiting nodes

	CJobSystem& _Owner;
	uint32_t    _Index = std::numeric_limits<uint32_t>().max();

public:

	CWorker(CJobSystem& Owner, uint32_t Index)
		: _Owner(Owner)
		, _Index(Index)
	{
	}

	void MainLoop();

	//???or in a CJobSystem, by index?
	// SetAffinity
	// SetCapabilities (mask - main thread, render context etc)
};

}
