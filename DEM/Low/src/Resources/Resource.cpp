#include "Resource.h"
#include <Resources/ResourceGenerator.h>
#include <Resources/ResourceLoader.h>

namespace Resources
{

CResource::CResource(): ByteSize(0), State(Rsrc_NotLoaded)
{
}
//--------------------------------------------------------------------

CResource::~CResource()
{
}
//--------------------------------------------------------------------

//!!!must be thread-safe!
void CResource::Init(PResourceObject NewObject, PResourceLoader NewLoader, PResourceGenerator NewGenerator)
{
	Object = NewObject;
	Loader = NewLoader;
	Generator = NewGenerator;
}
//--------------------------------------------------------------------

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