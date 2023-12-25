#pragma once
#include "WorkStealingQueue.h"
#include <thread>

// A worker thread wrapper. After construction, all its fields are intended for access from the contained thread.

namespace DEM::Jobs
{
class CJobSystem;

class CWorker final
{
protected:

	// CJob - to CJobSystem or separate header? Job = dependency counter + std::function

	//CWorkStealingQueue<CJob*>
	//CPoolAllocator<CJob> - ensure works well when the job is stolen, how to free job in the allocation thread, not in stealing thread?! Allocate aligned by cacheline.
	//Random generator
	//mutex and conditional variable for sleeping when there are no jobs; or a single cond var in a CJobSystem?! local queue can't be pushed in when the thread sleeps.

	CJobSystem* _pOwner = nullptr;
	uint32_t    _ID = std::numeric_limits<uint32_t>().max();
	std::thread _Thread;

public:

	CWorker(CWorker&& Other) noexcept = default;

	CWorker(CJobSystem& Owner, uint32_t ID, std::thread&& Thread)
		: _pOwner(&Owner)
		, _ID(ID)
		, _Thread(std::move(Thread))
	{
	}

	~CWorker();

	bool MainLoop();

	// SetAffinity
	// SetCapabilities (mask - main thread, render context etc)
};

}
