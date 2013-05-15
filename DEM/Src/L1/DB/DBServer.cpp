#include "DBServer.h"

#include <Events/EventManager.h>

int RegisterN2SQLiteVFS();
int UnregisterN2SQLiteVFS();

namespace DB
{
__ImplementClassNoFactory(DB::CDBServer, Core::CRefCounted);
__ImplementSingleton(DB::CDBServer);

CDBServer::CDBServer()
{
	__ConstructSingleton;
	n_assert(RegisterN2SQLiteVFS() == 0);
}
//---------------------------------------------------------------------

CDBServer::~CDBServer()
{
	n_assert(UnregisterN2SQLiteVFS() == 0);
	__DestructSingleton;
}
//---------------------------------------------------------------------

CAttrID CDBServer::RegisterAttrID(const char* Name, /*fourcc,*/ char Flags, const CData& DefaultVal)
{
	CStrID NameID = CStrID(Name);
	n_assert(!AttrIDRegistry.Contains(NameID));
	CAttributeID ID(NameID, Flags, DefaultVal);
	AttrIDRegistry.Add(NameID, ID);
	return AttrIDRegistry.Get(NameID);
}
//---------------------------------------------------------------------

} // namespace DB