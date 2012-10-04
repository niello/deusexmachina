#include "CoreServer.h"

#include <Core/RefCounted.h>
#include <kernel/nkernelserver.h>

//!!!only for msg box!
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#undef GetClassName

namespace Core
{
__ImplementSingleton(Core::CCoreServer);

nList CCoreServer::RefCountedList;

CCoreServer::CCoreServer(const nString& vendor, const nString& app):
	_IsOpen(false),
	pKernelServer(NULL)
{
	__ConstructSingleton;
	n_dbgmeminit();
	pKernelServer = n_new(nKernelServer);
}
//---------------------------------------------------------------------

CCoreServer::~CCoreServer()
{
	n_assert(!_IsOpen);

	CRefCounted* pObj;
	int LeakCount = 0;
	while (pObj = (CRefCounted*)RefCountedList.RemHead())
	{
		n_printf("Object at address '%lx' still referenced (refcount = %d), class '%s'\n",
			pObj, pObj->GetRefCount(), pObj->GetClassName());
		LeakCount++;
	}

	if (LeakCount > 0)
	{
		nString Msg;
		Msg.Format("There were %d objects still referenced, check log for details!", LeakCount);
		MessageBox(0, Msg.Get(), "DEM Core Message", MB_OK | MB_APPLMODAL | MB_SETFOREGROUND | MB_TOPMOST | MB_ICONINFORMATION);
	}

	n_delete(pKernelServer);
	pKernelServer = NULL;

	__DestructSingleton;
}
//---------------------------------------------------------------------

bool CCoreServer::Open()
{
	n_assert(pKernelServer);
	n_assert(!_IsOpen);
	_IsOpen = true;
	OK;
}
//---------------------------------------------------------------------

void CCoreServer::Close()
{
	n_assert(_IsOpen);
	n_assert(pKernelServer);
	_IsOpen = false;
}
//---------------------------------------------------------------------

} // namespace Core
