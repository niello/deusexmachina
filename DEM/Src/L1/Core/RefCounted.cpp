#include "RefCounted.h"

namespace Core
{
__ImplementRootClassNoFactory(Core::CRefCounted, 'RFCN');

#ifdef _DEBUG
CRefCountedList CRefCounted::List;
#endif

// NB: the destructor of derived classes MUST be virtual!
CRefCounted::~CRefCounted()
{
	n_assert(!RefCount);
#ifdef _DEBUG
	n_assert(ListIt);
	List.Remove(ListIt);
	ListIt = NULL;
#endif
}
//---------------------------------------------------------------------

#ifdef _DEBUG
void CRefCounted::DumpLeaks()
{
	if (List.IsEmpty()) n_dbgout("\n>>> NO REFCOUNT LEAKS\n\n\n");
	else
	{
		n_dbgout("\n\n\n******** REFCOUNTING LEAKS DETECTED:\n\n");
		CRefCountedList::CIterator It;
		for (CRefCountedList::CIterator It = List.Begin(); It != List.End(); It++)
		{
			CString Msg;
			Msg.Format("*** REFCOUNT LEAK: Object of class '%s' at address '0x%08lx', refcount is '%d'\n", 
				(*It)->GetClassName().CStr(),
				(*It),
				(*It)->GetRefCount());
			n_dbgout(Msg.CStr());
		}
		n_dbgout("\n******** END OF REFCOUNT LEAK REPORT\n\n\n");
		n_error("CRefCounted memory leaks detected");
	}
}
//---------------------------------------------------------------------
#endif

}