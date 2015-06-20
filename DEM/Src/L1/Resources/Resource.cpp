#include "Resource.h"

#include <Resources/ResourceObject.h>

namespace Resources
{

CResourceObject* CResource::GetObject()
{
	if (State == Rsrc_Loaded)
	{
		if (Object.IsValid() && Object->IsResourceValid()) return Object.GetUnsafe();
		else State = Rsrc_NotLoaded; //!!!may be const if not update state!
	}
	//if (Placeholder.IsValid() && Placeholder->IsResourceValid()) return Placeholder.GetUnsafe();
	return NULL;
}
//--------------------------------------------------------------------

}