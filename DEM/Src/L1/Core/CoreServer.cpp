#include "CoreServer.h"

#include <Core/Object.h>

namespace Core
{
__ImplementSingleton(Core::CCoreServer);

const CString CCoreServer::Mem_HighWaterSize("Mem_HighWaterSize");
const CString CCoreServer::Mem_TotalSize("Mem_TotalSize");
const CString CCoreServer::Mem_TotalCount("Mem_TotalCount");

CCoreServer::CCoreServer()
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

	// It dumps leaks to a file!
	//if (n_dbgmemdumpleaks() != 0)
	//	n_dbgout("n_dbgmemdumpleaks detected and dumped memory leaks");

	__DestructSingleton;
}
//---------------------------------------------------------------------

void CCoreServer::Trigger()
{
#ifdef DEM_STATS
	CMemoryStats Stats = n_dbgmemgetstats();
	CoreSrv->SetGlobal<int>(Mem_HighWaterSize, Stats.HighWaterSize);
	CoreSrv->SetGlobal<int>(Mem_TotalSize, Stats.TotalSize);
	CoreSrv->SetGlobal<int>(Mem_TotalCount, Stats.TotalCount);
#endif
}
//---------------------------------------------------------------------

}