#include "Object.h"

namespace Core
{
__ImplementRootClassNoFactory(Core::CObject, 'OBJT');

#ifdef _DEBUG
CObject::CObjList CObject::List;

void CObject::DumpLeaks()
{
	if (List.IsEmpty()) Core::DbgOut("\n>>> NO REFCOUNT LEAKS\n\n\n");
	else
	{
		Core::DbgOut("\n\n\n******** REFCOUNTING LEAKS DETECTED:\n\n");
		for (CObjList::CIterator It = List.Begin(); It != List.End(); It++)
		{
			Core::DbgOut("*** REFCOUNT LEAK: Object of class '%s' at address '0x%08lx', refcount is '%d'\n",
				(*It)->GetClassName().CStr(),
				(*It),
				(*It)->GetRefCount());
		}
		Core::DbgOut("\n******** END OF REFCOUNT LEAK REPORT\n\n\n");
		Core::Error("CObject memory leaks detected");
	}
}
//---------------------------------------------------------------------
#endif

}