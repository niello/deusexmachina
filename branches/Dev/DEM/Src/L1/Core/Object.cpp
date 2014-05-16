#include "Object.h"

namespace Core
{
__ImplementRootClassNoFactory(Core::CObject, 'OBJT');

#ifdef _DEBUG
CObject::CObjList CObject::List;

void CObject::DumpLeaks()
{
	if (List.IsEmpty()) Sys::DbgOut("\n>>> NO REFCOUNT LEAKS\n\n\n");
	else
	{
		Sys::DbgOut("\n\n\n******** REFCOUNTING LEAKS DETECTED:\n\n");
		for (CObjList::CIterator It = List.Begin(); It != List.End(); It++)
		{
			Sys::DbgOut("*** REFCOUNT LEAK: Object of class '%s' at address '0x%08lx', refcount is '%d'\n",
				(*It)->GetClassName().CStr(),
				(*It),
				(*It)->GetRefCount());
		}
		Sys::DbgOut("\n******** END OF REFCOUNT LEAK REPORT\n\n\n");
		Sys::Error("CObject memory leaks detected");
	}
}
//---------------------------------------------------------------------
#endif

}