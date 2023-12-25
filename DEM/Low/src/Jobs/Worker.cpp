#include "Worker.h"

namespace DEM::Jobs
{

CWorker::~CWorker()
{
	//???signal worker termination to cancel all jobs in a queue and terminate immediately?!
	if (_Thread.joinable()) _Thread.join();
}

bool CWorker::MainLoop()
{
	// loop here or only one iteration? for processing main thread queue in a main thread?
	//!!!if not loop here, rename to Step or something like that!
	return false;
}

}
