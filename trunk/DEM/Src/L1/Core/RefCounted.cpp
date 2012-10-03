#include "RefCounted.h"

#include "CoreServer.h"

namespace Core
{
ImplementRootRtti(Core::CRefCounted);

CRefCounted::CRefCounted(): RefCount(0)
{
	CCoreServer::RefCountedList.AddTail(this);
}
//---------------------------------------------------------------------

// NOTE: the destructor of derived classes MUST be virtual!
CRefCounted::~CRefCounted()
{
    n_assert(RefCount == 0);
    Remove();
}
//---------------------------------------------------------------------

} // namespace Core
