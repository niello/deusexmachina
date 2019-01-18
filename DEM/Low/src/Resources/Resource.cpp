#include "Resource.h"
#include <Resources/ResourceCreator.h>

namespace Resources
{
CResource::CResource(CStrID UID) : _UID(UID) {}
CResource::~CResource() {}

void CResource::SetCreator(IResourceCreator* pNewCreator)
{
	if (pNewCreator)
	{
		if (Object && (pNewCreator != Creator || !Object->IsA(pNewCreator->GetResultType())))
		{
			// unload resource if loaded
			// reload with a new creator if was loaded?
		}
	}

	Creator = pNewCreator;
}
//--------------------------------------------------------------------

CResourceObject* CResource::GetObject()
{
	if (State == EResourceState::Loaded)
	{
		if (Object && Object->IsResourceValid()) return Object.Get();
		else State = EResourceState::NotLoaded;
	}
	//if (Placeholder.IsValid() && Placeholder->IsResourceValid()) return Placeholder.Get();
	return nullptr;
}
//--------------------------------------------------------------------

CResourceObject* CResource::ValidateObject()
{
	if (State == EResourceState::Loaded)
	{
		if (Object && Object->IsResourceValid()) return Object.Get();
		else State = EResourceState::NotLoaded;
	}

	if (State == EResourceState::NotLoaded && Creator)
	{
		State = EResourceState::LoadingInProgress;
		Object = Creator->CreateResource(_UID);
		State = Object && Object->IsResourceValid() ? EResourceState::Loaded : EResourceState::LoadingFailed;
		return Object.Get();
	}

	return nullptr;
}
//--------------------------------------------------------------------

}