#include "Object.h"

namespace DEM::Core
{

#ifdef _DEBUG
CObject::CObjList CObject::List;

void CObject::DumpLeaks() //???dump CRefCounted leaks too? without class names.
{
	if (List.IsEmpty()) Sys::DbgOut("\n>>> CObject: NO REFCOUNT LEAKS\n\n");
	else
	{
		Sys::DbgOut("\n\n******** CObject: REFCOUNTING LEAKS DETECTED:\n\n");
		UPTR Number = 1;
		for (CObjList::CIterator It = List.Begin(); It != List.End(); ++It, ++Number)
		{
			Sys::DbgOut("*** REFCOUNT LEAK {}: Object of class '{}' at address '{}', refcount is '{}'\n"_format(
				Number,
				(*It)->GetClassName().c_str(),
				fmt::ptr(*It),
				(*It)->GetRefCount()));
		}
		Sys::DbgOut("\n******** CObject: END OF REFCOUNT LEAK REPORT\n\n");
		Sys::Error("CObject memory leaks detected");
	}
}
//---------------------------------------------------------------------
#endif

}
