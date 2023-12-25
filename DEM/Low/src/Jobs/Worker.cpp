#include "Worker.h"

namespace DEM::Jobs
{

bool CWorker::MainLoop()
{
	//!!!DBG TMP!
	ZoneScopedN("THREAD");
	using namespace std::chrono_literals;
	std::this_thread::sleep_for(400ms);

	// loop here or only one iteration? for processing main thread queue in a main thread?
	//!!!if not loop here, rename to Step or something like that!
	//!!!if loop, return void, not bool!
	return false;
}
//---------------------------------------------------------------------

}
