#include "CoreServer.h"

#include <Core/RefCounted.h>

namespace Core
{
__ImplementSingleton(Core::CCoreServer);

CCoreServer::CCoreServer(): _IsOpen(false)
{
	__ConstructSingleton;
	n_dbgmeminit();
}
//---------------------------------------------------------------------

CCoreServer::~CCoreServer()
{
	n_assert(!_IsOpen);

#ifdef _DEBUG
	CRefCounted::DumpLeaks();
#endif

	// It _dumps_ leaks!
	//if (n_dbgmemdumpleaks() != 0)
	//	n_dbgout("n_dbgmemdumpleaks detected and dumped memory leaks");

	__DestructSingleton;
}
//---------------------------------------------------------------------

bool CCoreServer::Open()
{
	n_assert(!_IsOpen);
	_IsOpen = true;
	OK;
}
//---------------------------------------------------------------------

void CCoreServer::Close()
{
	n_assert(_IsOpen);
	_IsOpen = false;
}
//---------------------------------------------------------------------

void CCoreServer::Trigger()
{
	nMemoryStats Stats = n_dbgmemgetstats();
	CoreSrv->SetGlobal<int>("Mem_HighWaterSize", Stats.HighWaterSize);
	CoreSrv->SetGlobal<int>("Mem_TotalSize", Stats.TotalSize);
	CoreSrv->SetGlobal<int>("Mem_TotalCount", Stats.TotalCount);
}
//---------------------------------------------------------------------

}