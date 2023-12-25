#include "Worker.h"

namespace DEM::Jobs
{

void CWorker::MainLoop()
{
	//!!!DBG TMP!
	ZoneScopedN("THREAD");
	using namespace std::chrono_literals;
	std::this_thread::sleep_for(400ms);

	//???!!!if termination, return right from loop?!
	// while (auto pJob = _Queue.Pop()) *pJob(/*this, pJob*/);

	//???!!!if termination, return right from loop?!
	// try steal from random victims, make some attempts, yelding intermediately

	// sleep until woken up by incoming tasks or until termination is requested
}
//---------------------------------------------------------------------

}
