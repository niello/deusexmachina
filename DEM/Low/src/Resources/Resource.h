#pragma once
#include <Resources/ResourceObject.h>
#include <Data/StringID.h>

// A container that wraps some actual resource object along with any information
// required for its management. This class is not intended to be a base class for
// specific resources. Inherit from a CResourceObject instead.
// Resource UID may consist of 2 parts and must have at least one of them.
// Format is "engine:path/to/file.ext#Id". '#' is a delimiter, you can't
// use it as an identifier part. System processes UIDs as follows:
// a) "engine:path/to/file.ext" is loaded from file with 1 file - 1 resource relation;
// b) "#Id" is generated procedurally and is not associated with a file;
// c) "engine:path/to/file.ext#Id" is intended for loading from files containing
//    multiple resources. Loader will use an "Id" part to select a subresource to load.

// TODO: can implement own refcounting and drop virtual table & destructor required by CRefCounted

namespace Resources
{
typedef Ptr<class IResourceCreator> PResourceCreator;
typedef Ptr<class CResource> PResource;

enum class EResourceState
{
	NotLoaded,
	LoadingRequested,
	LoadingInProgress,
	Loaded,
	LoadingFailed,
	LoadingCancelled
};

class CResource final : public Data::CRefCounted
{
protected:

	CStrID				_UID;
	EResourceState		_State = EResourceState::NotLoaded;
	PResourceObject		_Object;  // Actual object, such as a texture or a game object description
	PResourceCreator	_Creator; // For (re)creation of an actual resource object

	//PResourceObject	_Placeholder; //???here or register per RTTI in Mgr?
	//UPTR				_ByteSize = 0; //???or getter with redirection to PResourceObject?

public:

	CResource(CStrID UID);
	virtual ~CResource() override;

	template<class T>
	T*                GetObject();
	template<class T>
	T*                ValidateObject();

	CResourceObject*  GetObject();
	CResourceObject*  ValidateObject();
	void              Unload();

	//PJob ValidateObjectAsync(callback on finished); - doesn't create a job if already actual

	CStrID            GetUID() const { return _UID; }
	EResourceState    GetState() const { return _State; } //!!!must be thread-safe!
	bool              IsLoaded() const { return _State == EResourceState::Loaded; } //!!!must be thread-safe!
	IResourceCreator* GetCreator() const { return _Creator.Get(); }
	void              SetCreator(IResourceCreator* pNewCreator);
};

template<class T> inline T* CResource::GetObject()
{
	CResourceObject* pObj = GetObject();
	return (pObj && pObj->IsA<T>()) ? static_cast<T*>(pObj) : nullptr;
}
//--------------------------------------------------------------------

template<class T> inline T* CResource::ValidateObject()
{
	CResourceObject* pObj = ValidateObject();
	return (pObj && pObj->IsA<T>()) ? static_cast<T*>(pObj) : nullptr;
}
//--------------------------------------------------------------------

}
