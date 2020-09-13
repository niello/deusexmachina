#include "Resource.h"
#include <Resources/ResourceCreator.h>

namespace Resources
{
CResource::CResource(CStrID UID) : _UID(UID) {}
CResource::~CResource() = default;

void CResource::SetCreator(IResourceCreator* pNewCreator)
{
	if (_Object && pNewCreator && (pNewCreator != _Creator || !_Object->IsA(pNewCreator->GetResultType())))
		Unload();

	_Creator = pNewCreator;
}
//--------------------------------------------------------------------

CResourceObject* CResource::GetObject()
{
	if (_State == EResourceState::Loaded)
	{
		if (_Object && _Object->IsResourceValid()) return _Object.Get();
		else _State = EResourceState::NotLoaded;
	}
	//if (_Placeholder.IsValid() && _Placeholder->IsResourceValid()) return _Placeholder.Get();
	return nullptr;
}
//--------------------------------------------------------------------

CResourceObject* CResource::ValidateObject()
{
	if (_State == EResourceState::Loaded)
	{
		if (_Object && _Object->IsResourceValid()) return _Object.Get();
		else _State = EResourceState::NotLoaded;
	}

	if (_State == EResourceState::NotLoaded && _Creator)
	{
		_State = EResourceState::LoadingInProgress;
		_Object = _Creator->CreateResource(_UID);
		_State = _Object && _Object->IsResourceValid() ? EResourceState::Loaded : EResourceState::LoadingFailed;
		return _Object.Get();
	}

	return nullptr;
}
//--------------------------------------------------------------------

void CResource::Unload()
{
	_State = EResourceState::NotLoaded;
	_Object = nullptr;
}
//--------------------------------------------------------------------

}
