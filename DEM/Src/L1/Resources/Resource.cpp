#include "Resource.h"

namespace Resources
{

CResourceObject* CResource::GetObject() const
{
	if (State == Rsrc_Loaded)
	{
		if (Object.IsValidPtr() && Object->IsResourceValid()) return Object.GetUnsafe();
		else State = Rsrc_NotLoaded; //!!!may be const if not update state!
	}
	//if (Placeholder.IsValid() && Placeholder->IsResourceValid()) return Placeholder.GetUnsafe();
	return NULL;
}
//--------------------------------------------------------------------

}