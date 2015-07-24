#include "CoreServer.h"

#include <Core/Object.h>

namespace Core
{
__ImplementSingleton(Core::CCoreServer);

CCoreServer::CCoreServer():
	Mem_HighWaterSize("Mem_HighWaterSize"),
	Mem_TotalSize("Mem_TotalSize"),
	Mem_TotalCount("Mem_TotalCount")
{
	__ConstructSingleton;
	n_dbgmeminit();
}
//---------------------------------------------------------------------

CCoreServer::~CCoreServer()
{
#ifdef _DEBUG
	CObject::DumpLeaks();
#endif

	// It _dumps_ leaks!
	//if (n_dbgmemdumpleaks() != 0)
	//	n_dbgout("n_dbgmemdumpleaks detected and dumped memory leaks");

	__DestructSingleton;
}
//---------------------------------------------------------------------

void CCoreServer::Trigger()
{
	nMemoryStats Stats = n_dbgmemgetstats();
	CoreSrv->SetGlobal<int>(Mem_HighWaterSize, Stats.HighWaterSize);
	CoreSrv->SetGlobal<int>(Mem_TotalSize, Stats.TotalSize);
	CoreSrv->SetGlobal<int>(Mem_TotalCount, Stats.TotalCount);
}
//---------------------------------------------------------------------

}