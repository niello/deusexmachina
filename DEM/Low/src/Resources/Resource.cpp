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

DEM::Core::CObject* CResource::GetObject()
{
	if (_State == EResourceState::Loaded)
	{
		if (_Object) return _Object.Get();
		else _State = EResourceState::NotLoaded;
	}
	// FIXME: placeholder per resource TYPE?
	//if (_Placeholder) return _Placeholder.Get();
	return nullptr;
}
//--------------------------------------------------------------------

DEM::Core::CObject* CResource::ValidateObject()
{
	if (_State == EResourceState::Loaded)
	{
		if (_Object) return _Object.Get();
		else _State = EResourceState::NotLoaded;
	}

	if (_State == EResourceState::NotLoaded && _Creator)
	{
		ZoneScopedN("CreateResource");
		ZoneText(_UID.CStr(), std::strlen(_UID.CStr()));

		_State = EResourceState::LoadingInProgress;
		_Object = _Creator->CreateResource(_UID);
		_State = _Object ? EResourceState::Loaded : EResourceState::LoadingFailed;
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
